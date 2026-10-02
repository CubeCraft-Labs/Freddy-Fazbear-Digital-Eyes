#include "TrackingGate.h"

#include <assert.h>
#include <stdio.h>

int main() {
  tracking::Gate gate;
  assert(!gate.update(false, 0, 0, 0).valid);
  assert(!gate.update(true, 160, 100, 250).valid);
  assert(!gate.update(true, 165, 105, 500).valid);
  const auto first_lock = gate.update(true, 170, 110, 750);
  assert(first_lock.valid && !first_lock.held && first_lock.x == 170);

  // Two missed model results keep both eyes tracking the last known position.
  const auto missed = gate.update(false, 0, 0, 1000);
  assert(missed.valid && missed.held && missed.x == 170 && missed.y == 110);
  assert(gate.update(false, 0, 0, 1250).valid);
  const auto reacquired = gate.update(true, 173, 108, 1500);
  assert(reacquired.valid && !reacquired.held && reacquired.x == 173);

  // A new distant candidate cannot yank the gaze until confirmed.
  const auto other = gate.update(true, 290, 200, 1750);
  assert(other.valid && other.held && other.x == 173);
  assert(gate.update(true, 292, 201, 2000).held);
  const auto switched = gate.update(true, 294, 202, 2250);
  assert(switched.valid && !switched.held && switched.x == 294);

  // Once the person leaves, the brief hold expires and blue/IDLE returns.
  assert(gate.update(false, 0, 0, 3250).valid);
  assert(!gate.update(false, 0, 0, 3251).valid);
  assert(!gate.update(true, 160, 100, 3500).valid);
  assert(!gate.update(true, 162, 102, 3750).valid);
  assert(gate.update(true, 164, 104, 4000).valid);
  puts("tracking gate: confirmation, brief misses, candidate switch, expiry OK");
}
