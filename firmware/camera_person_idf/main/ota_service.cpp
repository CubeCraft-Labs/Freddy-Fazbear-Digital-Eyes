#include "ota_service.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/sha256.h"
#include "nvs_flash.h"
#include "../../shared/OtaAuth.h"
#include "../../shared/OtaSecrets.h"

namespace {
constexpr char kTag[] = "freddy_ota";
constexpr char kDevice[] = "camera";
constexpr char kImageTarget[] = "FREDDY_OTA_TARGET:camera";
constexpr int kPort = 3232;
char nonce[33] = {};
int64_t nonce_time_us = 0;
httpd_handle_t server = nullptr;

void new_nonce() {
  uint8_t random_bytes[16];
  esp_fill_random(random_bytes, sizeof(random_bytes));
  freddy_ota::to_hex(random_bytes, sizeof(random_bytes), nonce);
  nonce_time_us = esp_timer_get_time();
}

esp_err_t info_handler(httpd_req_t *req) {
  new_nonce();
  const esp_partition_t *slot = esp_ota_get_next_update_partition(nullptr);
  const esp_partition_t *running = esp_ota_get_running_partition();
  char body[180];
  snprintf(body, sizeof(body),
           "{\"device\":\"%s\",\"nonce\":\"%s\",\"slot_bytes\":%u,\"running_slot\":%d}",
           kDevice, nonce, slot ? static_cast<unsigned>(slot->size) : 0,
           running ? static_cast<int>(running->subtype) - ESP_PARTITION_SUBTYPE_APP_OTA_0 : -1);
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_sendstr(req, body);
}

bool read_header(httpd_req_t *req, const char *name, char *out, size_t capacity) {
  const size_t length = httpd_req_get_hdr_value_len(req, name);
  return length > 0 && length < capacity &&
         httpd_req_get_hdr_value_str(req, name, out, capacity) == ESP_OK;
}

void restart_task(void *) {
  vTaskDelay(pdMS_TO_TICKS(1000));
  esp_restart();
}

esp_err_t upload_handler(httpd_req_t *req) {
  char request_nonce[33], request_sha[65], signature[65];
  const esp_partition_t *slot = esp_ota_get_next_update_partition(nullptr);
  if (!slot || req->content_len <= 0 ||
      static_cast<size_t>(req->content_len) > slot->size ||
      !read_header(req, "X-Freddy-Nonce", request_nonce, sizeof(request_nonce)) ||
      !read_header(req, "X-Freddy-SHA256", request_sha, sizeof(request_sha)) ||
      !read_header(req, "X-Freddy-Signature", signature, sizeof(signature)) ||
      esp_timer_get_time() - nonce_time_us > 60000000 ||
      strcmp(request_nonce, nonce) != 0 ||
      !freddy_ota::verify(FREDDY_OTA_KEY, kDevice, nonce, request_sha,
                          req->content_len, signature)) {
    httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "OTA authorization failed");
    return ESP_FAIL;
  }
  nonce[0] = 0;  // Prevent replay even after an interrupted upload.
  esp_ota_handle_t handle;
  esp_err_t result = esp_ota_begin(slot, req->content_len, &handle);
  if (result != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
    return ESP_FAIL;
  }
  mbedtls_sha256_context sha_context;
  mbedtls_sha256_init(&sha_context);
  result = mbedtls_sha256_starts(&sha_context, 0) == 0 ? ESP_OK : ESP_FAIL;
  uint8_t buffer[2048];
  int remaining = req->content_len;
  unsigned timeouts = 0;
  while (result == ESP_OK && remaining > 0) {
    const int received = httpd_req_recv(
        req, reinterpret_cast<char *>(buffer),
        remaining < static_cast<int>(sizeof(buffer)) ? remaining : sizeof(buffer));
    if (received == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < 5) continue;
    if (received <= 0) {
      result = ESP_FAIL;
      break;
    }
    timeouts = 0;
    result = esp_ota_write(handle, buffer, received);
    if (result == ESP_OK && mbedtls_sha256_update(&sha_context, buffer, received) != 0)
      result = ESP_FAIL;
    remaining -= received;
  }
  uint8_t digest[32] = {};
  if (result == ESP_OK &&
      mbedtls_sha256_finish(&sha_context, digest) != 0) result = ESP_FAIL;
  mbedtls_sha256_free(&sha_context);
  char actual_sha[65];
  freddy_ota::to_hex(digest, sizeof(digest), actual_sha);
  if (result == ESP_OK &&
      !freddy_ota::constant_time_equal(actual_sha, request_sha, 64))
    result = ESP_FAIL;
  if (result == ESP_OK) result = esp_ota_end(handle);
  else esp_ota_abort(handle);
  if (result == ESP_OK) result = esp_ota_set_boot_partition(slot);
  if (result != ESP_OK) {
    ESP_LOGE(kTag, "OTA upload failed: %s", esp_err_to_name(result));
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "OTA image rejected");
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"ok\":true}");
  xTaskCreate(restart_task, "ota_restart", 2048, nullptr, 5, nullptr);
  return ESP_OK;
}

void wifi_event(void *, esp_event_base_t base, int32_t id, void *data) {
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    esp_wifi_connect();
  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    const auto *event = static_cast<ip_event_got_ip_t *>(data);
    ESP_LOGI(kTag, "OTA_READY device=%s image=%s ip=" IPSTR " port=%d",
             kDevice, kImageTarget, IP2STR(&event->ip_info.ip), kPort);
  }
}
}  // namespace

void ota_service_start() {
  if (!FREDDY_WIFI_SSID[0] || strlen(FREDDY_OTA_KEY) < 32) {
    ESP_LOGW(kTag, "OTA_DISABLED: configure firmware/private/ first");
    return;
  }
  esp_err_t result = nvs_flash_init();
  if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
      result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    result = nvs_flash_init();
  }
  ESP_ERROR_CHECK(result);
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&init));
  ESP_ERROR_CHECK(esp_event_handler_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, nullptr));
  wifi_config_t config = {};
  if (strlen(FREDDY_WIFI_SSID) >= sizeof(config.sta.ssid) ||
      strlen(FREDDY_WIFI_PASSWORD) >= sizeof(config.sta.password)) {
    ESP_LOGE(kTag, "Wi-Fi credentials exceed ESP32 limits");
    return;
  }
  strncpy(reinterpret_cast<char *>(config.sta.ssid), FREDDY_WIFI_SSID,
          sizeof(config.sta.ssid) - 1);
  strncpy(reinterpret_cast<char *>(config.sta.password), FREDDY_WIFI_PASSWORD,
          sizeof(config.sta.password) - 1);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());
  httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
  http_config.server_port = kPort;
  http_config.stack_size = 8192;
  http_config.lru_purge_enable = true;
  ESP_ERROR_CHECK(httpd_start(&server, &http_config));
  const httpd_uri_t info = {
      .uri = "/ota/info", .method = HTTP_GET, .handler = info_handler,
      .user_ctx = nullptr,
  };
  const httpd_uri_t upload = {
      .uri = "/ota", .method = HTTP_POST, .handler = upload_handler,
      .user_ctx = nullptr,
  };
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &info));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &upload));
}

void ota_service_confirm_healthy() {
  const esp_err_t result = esp_ota_mark_app_valid_cancel_rollback();
  if (result == ESP_OK) ESP_LOGI(kTag, "OTA_IMAGE_CONFIRMED");
}
