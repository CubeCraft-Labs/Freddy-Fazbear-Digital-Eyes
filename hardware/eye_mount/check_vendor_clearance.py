"""Check all official Waveshare STEP solids against the first-fit carrier/trim.

Usage: uv run --no-project --with build123d python \
  hardware/eye_mount/check_vendor_clearance.py /path/to/ESP32-S3-Touch-LCD-2_1_asm.stp
"""

import sys
from pathlib import Path

from build123d import Pos, Rot, import_step

OUT = Path(__file__).resolve().parent
board = import_step(sys.argv[1]).solids()
print(f"Checking {len(board)} vendor solids")


def bbox_overlap(a, b):
    return all(getattr(a.min, k) < getattr(b.max, k)
               and getattr(b.min, k) < getattr(a.max, k)
               for k in ("X", "Y", "Z"))


for side, x in (("left", -30.0), ("right", 56.0)):
    mounts = [import_step(str(OUT / f"{side}_{name}.step"))
              for name in ("carrier", "trim_gauge")]
    clashes = []
    for index, solid in enumerate(board):
        placed = Pos(x, -15.0, 196.0) * Rot(90, 0, 0) * solid
        for kind, mount in zip(("carrier", "trim"), mounts):
            if not bbox_overlap(placed.bounding_box(), mount.bounding_box()):
                continue
            overlap = placed & mount
            if overlap is not None and overlap.volume > 0.01:
                clashes.append((index, kind, round(overlap.volume, 3)))
    print(f"{side}: {len(clashes)} board/mount clashes {clashes[:30]}")
