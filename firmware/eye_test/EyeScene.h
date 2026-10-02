#pragma once

#include <stdint.h>

// Creates a moving and blinking eye on the Waveshare 480x480 display.
void EyeScene_Create();

// Normalized camera-image coordinates. A valid target switches to the red eye;
// an invalid target returns to blue. Call from the LVGL loop task.
void EyeScene_SetGaze(uint16_t x, uint16_t y, bool valid);
