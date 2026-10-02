#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "Display_ST7701.h"
#include "LVGL_Driver.h"
#include "EyeScene.h"
#include "OtaService.h"
#include "../shared/GazeProtocol.h"

#ifndef EYE_BOARD_NUMBER
#define EYE_BOARD_NUMBER 1
#endif

namespace {
constexpr int kGazeRxPin = 44;
constexpr int kGazeBaud = 115200;
HardwareSerial gaze_serial(1);
gaze::Receiver gaze_receiver;
bool gaze_active = false;
uint32_t last_status_ms = 0;
}

void setup() {
  I2C_Init();
  TCA9554PWR_Init(0x00);
  Set_EXIO(EXIO_PIN8, Low);
  LCD_Init();
  Lvgl_Init();
  EyeScene_Create();
  Serial.begin(115200);
  gaze_serial.begin(kGazeBaud, SERIAL_8N1, kGazeRxPin, -1);
  OtaService_Begin();
}

void loop() {
  if (millis() - last_status_ms >= 5000) {
    last_status_ms = millis();
    if (Serial) Serial.printf("GAZE_READY board=%d recent=%d bad_format=%lu bad_sequence=%lu\n",
                              EYE_BOARD_NUMBER, gaze_receiver.recent(millis()),
                              static_cast<unsigned long>(gaze_receiver.rejected_format()),
                              static_cast<unsigned long>(gaze_receiver.rejected_sequence()));
  }
  gaze::Frame command = {};
  while (gaze_serial.available()) {
    const uint32_t old_format_rejections = gaze_receiver.rejected_format();
    const uint32_t old_sequence_rejections = gaze_receiver.rejected_sequence();
    if (gaze_receiver.feed(static_cast<uint8_t>(gaze_serial.read()),
                           millis(), &command)) {
      EyeScene_SetGaze(command.x, command.y, command.valid);
      gaze_active = command.valid;
      if (Serial) Serial.printf("GAZE_ACCEPT board=%d seq=%u x=%u y=%u valid=%d\n",
                                EYE_BOARD_NUMBER, command.sequence, command.x,
                                command.y, command.valid);
    }
    if (gaze_receiver.rejected_format() != old_format_rejections)
      if (Serial) Serial.printf("GAZE_REJECT board=%d reason=format\n", EYE_BOARD_NUMBER);
    if (gaze_receiver.rejected_sequence() != old_sequence_rejections)
      if (Serial) Serial.printf("GAZE_REJECT board=%d reason=sequence\n", EYE_BOARD_NUMBER);
  }
  if (gaze_active && !gaze_receiver.recent(millis())) {
    EyeScene_SetGaze(500, 500, false);
    gaze_active = false;
    if (Serial) Serial.printf("GAZE_TIMEOUT board=%d\n", EYE_BOARD_NUMBER);
  }
  Lvgl_Loop();
  OtaService_Loop();
  vTaskDelay(pdMS_TO_TICKS(5));
}
