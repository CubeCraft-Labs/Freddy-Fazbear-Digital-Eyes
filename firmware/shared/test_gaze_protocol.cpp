#include "GazeProtocol.h"

#include <assert.h>
#include <stdio.h>

namespace {
bool feed_frame(gaze::Receiver *receiver, const uint8_t *bytes,
                uint32_t now_ms, gaze::Frame *out) {
  bool accepted = false;
  for (size_t i = 0; i < gaze::kFrameSize; ++i) {
    accepted |= receiver->feed(bytes[i], now_ms, out);
  }
  return accepted;
}
}  // namespace

int main() {
  gaze::Frame sent = {65535, 0, 1000, true};
  uint8_t bytes[gaze::kFrameSize];
  gaze::encode(sent, bytes);
  gaze::Frame received = {};
  assert(gaze::decode(bytes, &received));
  assert(received.sequence == sent.sequence && received.x == 0 &&
         received.y == 1000 && received.valid);

  gaze::Receiver receiver;
  assert(feed_frame(&receiver, bytes, 100, &received));
  assert(receiver.recent(850));
  assert(!receiver.recent(851));
  assert(!feed_frame(&receiver, bytes, 200, &received));  // duplicate
  assert(receiver.rejected_sequence() == 1);

  sent.sequence = 0;  // wrap-around
  sent.x = 500;
  gaze::encode(sent, bytes);
  assert(feed_frame(&receiver, bytes, 200, &received));
  assert(received.sequence == 0 && received.x == 500);

  sent.sequence = 65535;  // out of order
  gaze::encode(sent, bytes);
  assert(!feed_frame(&receiver, bytes, 300, &received));
  assert(receiver.rejected_sequence() == 2);
  assert(feed_frame(&receiver, bytes, 1000, &received));  // reboot after timeout

  bytes[2] = 2;  // unsupported version
  assert(!gaze::decode(bytes, &received));
  gaze::encode(sent, bytes);
  bytes[6] ^= 1;  // bad checksum
  assert(!feed_frame(&receiver, bytes, 1100, &received));
  assert(receiver.rejected_format() == 1);

  sent.sequence = 1;
  sent.valid = false;
  gaze::encode(sent, bytes);
  receiver.feed(0xA5, 1100, &received);  // truncated preamble
  receiver.feed(0x00, 1100, &received);
  assert(feed_frame(&receiver, bytes, 1100, &received));
  assert(!received.valid);

  sent.x = 1001;
  gaze::encode(sent, bytes);
  assert(!gaze::decode(bytes, &received));
  puts("gaze protocol: round trip, CRC, version, range, resync, sequence, timeout OK");
}
