#include "OtaService.h"

#include <cstring>
#include "WiFi.h"
#include "WebServer.h"
#include "Update.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "mbedtls/sha256.h"
#include "../shared/OtaAuth.h"
#include "../shared/OtaSecrets.h"

#ifndef EYE_BOARD_NUMBER
#define EYE_BOARD_NUMBER 1
#endif

namespace {
constexpr uint16_t kPort = 3232;
#if EYE_BOARD_NUMBER == 2
constexpr char kDevice[] = "eye-2";
constexpr char kImageTarget[] = "FREDDY_OTA_TARGET:eye-2";
#else
constexpr char kDevice[] = "eye-1";
constexpr char kImageTarget[] = "FREDDY_OTA_TARGET:eye-1";
#endif
WebServer server(kPort);
char nonce[33] = {};
char expected_sha[65] = {};
uint32_t nonce_time = 0;
uint32_t retry_time = 0;
uint32_t status_time = 0;
uint32_t restart_time = 0;
size_t expected_length = 0;
bool listening = false;
bool upload_ok = false;
bool upload_started = false;
mbedtls_sha256_context sha_context;

void new_nonce() {
  uint8_t random_bytes[16];
  esp_fill_random(random_bytes, sizeof(random_bytes));
  freddy_ota::to_hex(random_bytes, sizeof(random_bytes), nonce);
  nonce_time = millis();
}

void send_info() {
  new_nonce();
  const esp_partition_t *slot = esp_ota_get_next_update_partition(nullptr);
  const esp_partition_t *running = esp_ota_get_running_partition();
  char body[180];
  snprintf(body, sizeof(body),
           "{\"device\":\"%s\",\"nonce\":\"%s\",\"slot_bytes\":%u,\"running_slot\":%d}",
           kDevice, nonce, slot ? static_cast<unsigned>(slot->size) : 0,
           running ? static_cast<int>(running->subtype) - ESP_PARTITION_SUBTYPE_APP_OTA_0 : -1);
  server.send(200, "application/json", body);
}

void handle_raw() {
  HTTPRaw &raw = server.raw();
  if (raw.status == RAW_START) {
    upload_ok = false;
    upload_started = false;
    const String length_text = server.header("Content-Length");
    const String nonce_text = server.header("X-Freddy-Nonce");
    const String sha_text = server.header("X-Freddy-SHA256");
    const String signature = server.header("X-Freddy-Signature");
    char *end = nullptr;
    const unsigned long parsed = strtoul(length_text.c_str(), &end, 10);
    const esp_partition_t *slot = esp_ota_get_next_update_partition(nullptr);
    if (!slot || !end || *end || !parsed || parsed > slot->size ||
        millis() - nonce_time > 60000 || nonce_text != nonce ||
        !freddy_ota::verify(FREDDY_OTA_KEY, kDevice, nonce,
                            sha_text.c_str(), parsed, signature.c_str())) return;
    expected_length = parsed;
    memcpy(expected_sha, sha_text.c_str(), 65);
    nonce[0] = 0;  // One use, even when the transfer fails.
    if (!Update.begin(expected_length)) return;
    mbedtls_sha256_init(&sha_context);
    if (mbedtls_sha256_starts(&sha_context, 0) != 0) {
      Update.abort();
      mbedtls_sha256_free(&sha_context);
      return;
    }
    upload_started = true;
  } else if (raw.status == RAW_WRITE && upload_started) {
    if (Update.write(raw.buf, raw.currentSize) != raw.currentSize ||
        mbedtls_sha256_update(&sha_context, raw.buf, raw.currentSize) != 0) {
      Update.abort();
      mbedtls_sha256_free(&sha_context);
      upload_started = false;
    }
  } else if (raw.status == RAW_END && upload_started) {
    uint8_t digest[32] = {};
    char actual[65];
    const bool hash_ok =
        raw.totalSize == expected_length &&
        mbedtls_sha256_finish(&sha_context, digest) == 0;
    mbedtls_sha256_free(&sha_context);
    freddy_ota::to_hex(digest, sizeof(digest), actual);
    upload_ok = hash_ok &&
                freddy_ota::constant_time_equal(actual, expected_sha, 64);
    if (upload_ok) upload_ok = Update.end();
    else Update.abort();
    upload_started = false;
  } else if (raw.status == RAW_ABORTED && upload_started) {
    Update.abort();
    mbedtls_sha256_free(&sha_context);
    upload_started = false;
  }
}

void finish_upload() {
  if (upload_ok) {
    server.send(200, "application/json", "{\"ok\":true}");
    restart_time = millis() + 1000;
  } else {
    server.send(400, "application/json", "{\"ok\":false}");
  }
}
}  // namespace

void OtaService_Begin() {
  if (!FREDDY_WIFI_SSID[0] || strlen(FREDDY_OTA_KEY) < 32) {
    Serial.println("OTA_DISABLED: configure firmware/private/ first");
    return;
  }
  const char *headers[] = {
      "Content-Length", "X-Freddy-Nonce", "X-Freddy-SHA256",
      "X-Freddy-Signature"};
  server.collectHeaders(headers, 4);
  server.on("/ota/info", HTTP_GET, send_info);
  server.on("/ota", HTTP_POST, finish_upload, handle_raw);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(EYE_BOARD_NUMBER == 2 ? "freddy-eye-2" : "freddy-eye-1");
  WiFi.setSleep(false);
  WiFi.begin(FREDDY_WIFI_SSID, FREDDY_WIFI_PASSWORD);
  retry_time = millis();
}

void OtaService_Loop() {
  if (!FREDDY_WIFI_SSID[0] || strlen(FREDDY_OTA_KEY) < 32) return;
  if (restart_time && static_cast<int32_t>(millis() - restart_time) >= 0)
    ESP.restart();
  if (millis() - status_time >= 10000) {
    status_time = millis();
    if (Serial) Serial.printf("OTA_WIFI device=%s status=%d ip=%s\n",
                              kDevice, WiFi.status(),
                              WiFi.localIP().toString().c_str());
  }
  if (WiFi.status() != WL_CONNECTED) {
    if (listening) {
      server.stop();
      listening = false;
    }
    if (millis() - retry_time > 10000) {
      WiFi.reconnect();
      retry_time = millis();
    }
    return;
  }
  if (!listening) {
    server.begin();
    listening = true;
    Serial.printf("OTA_READY device=%s image=%s ip=%s port=%u\n",
                  kDevice, kImageTarget, WiFi.localIP().toString().c_str(), kPort);
  }
  server.handleClient();
}
