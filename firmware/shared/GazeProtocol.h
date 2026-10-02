#pragma once

#include <stddef.h>
#include <stdint.h>

// A fixed-size, little-endian UART frame shared by the camera and both eyes.
namespace gaze {
constexpr uint8_t kMagic0 = 0xA5;
constexpr uint8_t kMagic1 = 0x5A;
constexpr uint8_t kVersion = 1;
constexpr uint8_t kValid = 0x01;
constexpr size_t kFrameSize = 12;
constexpr uint32_t kTimeoutMs = 750;

struct Frame {
  uint16_t sequence;
  uint16_t x;  // 0..1000, left to right in the camera image
  uint16_t y;  // 0..1000, top to bottom in the camera image
  bool valid;
};

inline uint16_t crc16(const uint8_t *bytes, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(bytes[i]) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

inline void encode(const Frame &frame, uint8_t out[kFrameSize]) {
  out[0] = kMagic0;
  out[1] = kMagic1;
  out[2] = kVersion;
  out[3] = frame.valid ? kValid : 0;
  out[4] = static_cast<uint8_t>(frame.sequence);
  out[5] = static_cast<uint8_t>(frame.sequence >> 8);
  out[6] = static_cast<uint8_t>(frame.x);
  out[7] = static_cast<uint8_t>(frame.x >> 8);
  out[8] = static_cast<uint8_t>(frame.y);
  out[9] = static_cast<uint8_t>(frame.y >> 8);
  const uint16_t crc = crc16(out, kFrameSize - 2);
  out[10] = static_cast<uint8_t>(crc);
  out[11] = static_cast<uint8_t>(crc >> 8);
}

inline bool decode(const uint8_t bytes[kFrameSize], Frame *out) {
  if (bytes[0] != kMagic0 || bytes[1] != kMagic1 ||
      bytes[2] != kVersion || (bytes[3] & ~kValid) != 0) return false;
  const uint16_t actual_crc = static_cast<uint16_t>(bytes[10]) |
                              (static_cast<uint16_t>(bytes[11]) << 8);
  if (crc16(bytes, kFrameSize - 2) != actual_crc) return false;
  const uint16_t x = static_cast<uint16_t>(bytes[6]) |
                     (static_cast<uint16_t>(bytes[7]) << 8);
  const uint16_t y = static_cast<uint16_t>(bytes[8]) |
                     (static_cast<uint16_t>(bytes[9]) << 8);
  if (x > 1000 || y > 1000) return false;
  out->sequence = static_cast<uint16_t>(bytes[4]) |
                  (static_cast<uint16_t>(bytes[5]) << 8);
  out->x = x;
  out->y = y;
  out->valid = (bytes[3] & kValid) != 0;
  return true;
}

class Receiver {
 public:
  // Returns true only for a complete, valid, fresh frame.
  bool feed(uint8_t byte, uint32_t now_ms, Frame *out) {
    if (length_ == 0 && byte != kMagic0) return false;
    if (length_ == 1 && byte != kMagic1) {
      length_ = byte == kMagic0 ? 1 : 0;
      return false;
    }
    bytes_[length_++] = byte;
    if (length_ < kFrameSize) return false;

    Frame decoded = {};
    const bool well_formed = decode(bytes_, &decoded);
    if (well_formed) {
      length_ = 0;
    } else {
      ++rejected_format_;
      // Keep an embedded preamble, or a trailing first magic byte.
      size_t start = 1;
      while (start < kFrameSize &&
             (bytes_[start] != kMagic0 ||
              (start + 1 < kFrameSize && bytes_[start + 1] != kMagic1))) {
        ++start;
      }
      length_ = kFrameSize - start;
      for (size_t i = 0; i < length_; ++i) bytes_[i] = bytes_[start + i];
    }
    if (!well_formed) return false;

    if (has_sequence_ && static_cast<uint32_t>(now_ms - last_ms_) <= kTimeoutMs) {
      const uint16_t advance = static_cast<uint16_t>(decoded.sequence - last_sequence_);
      if (advance == 0 || advance >= 0x8000) {
        ++rejected_sequence_;
        return false;
      }
    }
    has_sequence_ = true;
    last_sequence_ = decoded.sequence;
    last_ms_ = now_ms;
    *out = decoded;
    return true;
  }

  bool recent(uint32_t now_ms) const {
    return has_sequence_ && static_cast<uint32_t>(now_ms - last_ms_) <= kTimeoutMs;
  }

  uint32_t rejected_format() const { return rejected_format_; }
  uint32_t rejected_sequence() const { return rejected_sequence_; }

 private:
  uint8_t bytes_[kFrameSize] = {};
  size_t length_ = 0;
  bool has_sequence_ = false;
  uint16_t last_sequence_ = 0;
  uint32_t last_ms_ = 0;
  uint32_t rejected_format_ = 0;
  uint32_t rejected_sequence_ = 0;
};
}  // namespace gaze
