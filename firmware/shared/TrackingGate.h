#pragma once

#include <stdint.h>
#include <stdlib.h>

// One camera-side decision is broadcast to both eyes so brief model misses do
// not make their appearance and gaze repeatedly switch between TRACK and IDLE.
namespace tracking {
constexpr unsigned kConfirmFrames = 3;
constexpr uint32_t kTargetHoldMs = 1000;

struct Decision {
  int x;
  int y;
  bool valid;
  bool held;
};

class Gate {
 public:
  Decision update(bool detected, int x, int y, uint32_t now_ms) {
    if (detected) {
      if (candidate_count_ && abs(x - candidate_x_) <= 80 &&
          abs(y - candidate_y_) <= 60) {
        if (candidate_count_ < kConfirmFrames) ++candidate_count_;
      } else {
        candidate_count_ = 1;
      }
      candidate_x_ = x;
      candidate_y_ = y;
      if (candidate_count_ >= kConfirmFrames) {
        locked_ = true;
        last_confirmed_ms_ = now_ms;
        locked_x_ = x;
        locked_y_ = y;
        return {x, y, true, false};
      }
    }

    if (locked_ && static_cast<uint32_t>(now_ms - last_confirmed_ms_) <=
                       kTargetHoldMs) {
      return {locked_x_, locked_y_, true, true};
    }

    locked_ = false;
    if (!detected) {
      candidate_count_ = 0;
      candidate_x_ = -1;
      candidate_y_ = -1;
      return {500, 500, false, false};
    }
    return {x, y, false, false};
  }

 private:
  unsigned candidate_count_ = 0;
  int candidate_x_ = -1;
  int candidate_y_ = -1;
  bool locked_ = false;
  int locked_x_ = 500;
  int locked_y_ = 500;
  uint32_t last_confirmed_ms_ = 0;
};
}  // namespace tracking
