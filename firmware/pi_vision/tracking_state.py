"""Small Pi-side gate to keep both eyes on one person through brief misses."""

from __future__ import annotations


class TrackingState:
    def __init__(self, confirm_hits: int = 2, hold_seconds: float = 2.0) -> None:
        if confirm_hits < 1 or hold_seconds <= 0:
            raise ValueError("confirmation and hold must be positive")
        self.confirm_hits = confirm_hits
        self.hold_seconds = hold_seconds
        self.hits = 0
        self.last_seen: float | None = None
        self.valid = False
        self.x = 500
        self.y = 500

    def update(self, candidate: tuple[int, int] | None, now: float) -> tuple[int, int, bool]:
        if candidate is not None:
            x, y = candidate
            if not 0 <= x <= 1000 or not 0 <= y <= 1000:
                raise ValueError("candidate gaze coordinates out of range")
            if self.last_seen is None or now - self.last_seen > 0.75:
                self.hits = 0
            self.hits += 1
            self.last_seen = now
            if self.valid or self.hits >= self.confirm_hits:
                self.valid = True
                self.x, self.y = x, y
        elif self.last_seen is not None:
            if self.valid and now - self.last_seen >= self.hold_seconds:
                self.valid = False
                self.x = self.y = 500
            if not self.valid and now - self.last_seen > 0.75:
                self.hits = 0
        return self.x, self.y, self.valid
