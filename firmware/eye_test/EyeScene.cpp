#include "EyeScene.h"
#include "FreddyIris.h"

#include <cmath>
#include <lvgl.h>

#ifndef EYE_BOARD_NUMBER
#define EYE_BOARD_NUMBER 1
#endif

namespace {
constexpr int kDisplaySize = 480;
constexpr int kIrisSize = 178;
constexpr int kIrisRadius = kIrisSize / 2;
constexpr uint32_t kBlinkPeriodMs = 4600;
constexpr uint32_t kBlinkDurationMs = 300;

lv_obj_t *iris = nullptr;
lv_obj_t *red_iris = nullptr;
lv_obj_t *red_halo = nullptr;
lv_obj_t *sclera = nullptr;
lv_obj_t *upper_lid = nullptr;
lv_obj_t *lower_lid = nullptr;
lv_obj_t *status_label = nullptr;
bool tracking_red = false;
bool gaze_valid = false;
uint16_t gaze_x = 500;
uint16_t gaze_y = 500;
float drawn_x = 240.0f;
float drawn_y = 240.0f;

lv_obj_t *make_circle(lv_obj_t *parent, int size, uint32_t color) {
  lv_obj_t *obj = lv_obj_create(parent);
  lv_obj_set_size(obj, size, size);
  lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_outline_width(obj, 0, 0);
  lv_obj_set_style_shadow_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  return obj;
}

lv_obj_t *make_lid(lv_obj_t *parent) {
  lv_obj_t *obj = lv_obj_create(parent);
  lv_obj_set_style_radius(obj, 0, 0);
  lv_obj_set_style_bg_color(obj, lv_color_hex(0x743824), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_outline_width(obj, 0, 0);
  lv_obj_set_style_shadow_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  return obj;
}

lv_obj_t *make_red_layer(lv_obj_t *parent, int size, uint32_t color,
                         lv_opa_t opacity) {
  lv_obj_t *layer = make_circle(parent, size, color);
  lv_obj_set_style_bg_opa(layer, opacity, 0);
  lv_obj_set_pos(layer, (kIrisSize - size) / 2, (kIrisSize - size) / 2);
  return layer;
}

void eye_tick(lv_timer_t *) {
  const uint32_t elapsed_ms = lv_tick_get();
  const float seconds = static_cast<float>(elapsed_ms) / 1000.0f;

  // Easing continues between camera updates and back into idle after loss.
  const float desired_x = gaze_valid
      ? 240.0f + (static_cast<int>(gaze_x) - 500) * 0.104f
      : 240.0f + 52.0f * sinf(seconds * 0.86f);
  const float desired_y = gaze_valid
      ? 240.0f + (static_cast<int>(gaze_y) - 500) * 0.062f
      : 240.0f + 31.0f * sinf(seconds * 0.53f + 0.8f);
  drawn_x += (desired_x - drawn_x) * 0.20f;
  drawn_y += (desired_y - drawn_y) * 0.20f;
  const int iris_x = static_cast<int>(drawn_x);
  const int iris_y = static_cast<int>(drawn_y);
  lv_obj_set_pos(iris, iris_x - kIrisRadius, iris_y - kIrisRadius);
  lv_obj_set_pos(red_iris, iris_x - kIrisRadius, iris_y - kIrisRadius);
  if (tracking_red) {
    const int pulse = static_cast<int>(14.0f * sinf(seconds * 4.5f));
    lv_obj_set_style_bg_opa(red_halo, 55 + pulse, 0);
  }

  const uint32_t phase_ms = elapsed_ms % kBlinkPeriodMs;
  int lid_height = 1;
  if (phase_ms < kBlinkDurationMs) {
    const float phase = static_cast<float>(phase_ms) / kBlinkDurationMs;
    const float closure = sinf(phase * 3.14159265f);
    lid_height += static_cast<int>(240.0f * closure);
  }
  lv_obj_set_size(upper_lid, kDisplaySize, lid_height);
  lv_obj_set_size(lower_lid, kDisplaySize, lid_height);
  lv_obj_set_pos(lower_lid, 0, kDisplaySize - lid_height);
}
void set_tracking_red(bool enabled) {
  if (tracking_red == enabled) return;
  tracking_red = enabled;
  if (enabled) {
    lv_obj_add_flag(iris, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(red_iris, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(sclera, lv_color_hex(0x21191B), 0);
  } else {
    lv_obj_add_flag(red_iris, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(iris, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(sclera, lv_color_hex(0xF5F4EA), 0);
  }
}
}  // namespace

void EyeScene_SetGaze(uint16_t x, uint16_t y, bool valid) {
  if (gaze_valid != valid && status_label) {
#if EYE_BOARD_NUMBER == 2
    lv_label_set_text(status_label, valid ? "BOARD 2 TRACK" : "BOARD 2 IDLE");
#else
    lv_label_set_text(status_label, valid ? "BOARD 1 TRACK" : "BOARD 1 IDLE");
#endif
  }
  gaze_x = x;
  gaze_y = y;
  gaze_valid = valid;
  set_tracking_red(valid);
}

void EyeScene_Create() {
  lv_obj_t *screen = lv_scr_act();
  lv_obj_clean(screen);
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x39271D), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *sclera_outline = make_circle(screen, 444, 0x743824);
  lv_obj_set_pos(sclera_outline, 18, 18);
  sclera = make_circle(screen, 432, 0xF5F4EA);
  lv_obj_set_pos(sclera, 24, 24);

  iris = lv_img_create(screen);
  lv_img_set_src(iris, &kFreddyIris);
  lv_obj_clear_flag(iris, LV_OBJ_FLAG_SCROLLABLE);

  red_iris = lv_obj_create(screen);
  lv_obj_set_size(red_iris, kIrisSize, kIrisSize);
  lv_obj_set_style_bg_opa(red_iris, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(red_iris, 0, 0);
  lv_obj_set_style_pad_all(red_iris, 0, 0);
  lv_obj_clear_flag(red_iris, LV_OBJ_FLAG_SCROLLABLE);
  make_red_layer(red_iris, kIrisSize, 0x150909, LV_OPA_COVER);
  make_red_layer(red_iris, 144, 0x6E0B08, 34);
  red_halo = make_red_layer(red_iris, 116, 0xBE1B13, 55);
  make_red_layer(red_iris, 82, 0xE22D1E, 90);
  make_red_layer(red_iris, 48, 0xFA412B, 165);
  make_red_layer(red_iris, 28, 0xFF3A2B, LV_OPA_COVER);
  make_red_layer(red_iris, 13, 0xFFF2E3, LV_OPA_COVER);
  lv_obj_add_flag(red_iris, LV_OBJ_FLAG_HIDDEN);

  status_label = lv_label_create(screen);
#if EYE_BOARD_NUMBER == 2
  lv_label_set_text(status_label, "BOARD 2 IDLE");
#else
  lv_label_set_text(status_label, "BOARD 1 IDLE");
#endif
  lv_obj_set_style_text_color(status_label, lv_color_hex(0x3F5361), 0);
  lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 48);

  upper_lid = make_lid(screen);
  lower_lid = make_lid(screen);

  eye_tick(nullptr);
  lv_timer_create(eye_tick, 30, nullptr);
}
