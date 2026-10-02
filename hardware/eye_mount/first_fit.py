"""Export a *test-fit* eye-board carrier and cosmetic trim in mask STEP coordinates.

Run: uv run --no-project --with build123d python hardware/eye_mount/first_fit.py

The three mask pads per eye are modeled against the *CAD* shell, not the glued
print. Do not bond them to the mask until board/cable clearance and fit are
checked on the real assembly. Units are millimetres.
"""

import math
from pathlib import Path

from build123d import (
    Align, Box, Compound, Cylinder, Pos, Rot, Vector, export_step, export_stl,
    import_step,
)


ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
MASK = import_step(str(ROOT / "stls/freddy_fazbear_mask.step"))
HEAD_HALVES = MASK.solids()[:2]

# The official Waveshare CAD has 2 mm nominal bores in the four rear posts.
# Their centers are not a square. These are in the vendor board's local XY.
BOARD_POSTS = [(-29.5, 13.63818, -6.0), (29.5, 13.63818, -6.0),
               (-18.0, -27.06012, -6.1), (18.0, -27.06012, -6.1)]
BOARD_ORIGIN_Y = -15.0
EYE_CENTERS = [(-30.0, 196.0), (56.0, 196.0)]


def cyl(radius, height, x=0, y=0, z=0):
    return Pos(x, y, z) * Cylinder(
        radius, height, align=(Align.CENTER, Align.CENTER, Align.MIN)
    )


def arm(a, b, width, z0, height):
    """A straight XY rib joining a point on the carrier ring to an anchor."""
    d = Vector(b[0] - a[0], b[1] - a[1])
    angle = math.degrees(math.atan2(d.Y, d.X))
    return (Pos((a[0] + b[0]) / 2, (a[1] + b[1]) / 2, z0 + height / 2)
            * Rot(0, 0, angle) * Box(d.length, width, height))


def radial_slot(x, y, radius=1.8, travel=5.0):
    """Through-slot, with 5 mm adjustment along its radial direction."""
    r = (x * x + y * y) ** 0.5
    ux, uy = x / r, y / r
    a = (x - ux * travel / 2, y - uy * travel / 2)
    b = (x + ux * travel / 2, y + uy * travel / 2)
    return (cyl(radius, 5, *a, -13)
            + cyl(radius, 5, *b, -13)
            + arm(a, b, 2 * radius, -13, 5))


def carrier(outer_sign):
    # Rear carrier is an open ring, well behind the PCB parts. Its four small
    # stand-offs reach the vendor board posts at local Z=-6.0. Screw from back.
    part = cyl(37.0, 3.0, z=-12.0) - cyl(29.0, 5.0, z=-13.0)
    for x, y, back_z in BOARD_POSTS:
        part += cyl(3.5, back_z + 12.0, x, y, -12.0)
    # Two brow attachments and one lower/outer cheek attachment. The lateral
    # temple surface changes depth by >20 mm under a 12 mm pad; the lower
    # cheek is smoother, though it needs a much longer stand-off sleeve.
    anchors = [(-20.0, 42.0), (20.0, 42.0), (outer_sign * 20.0, -38.0)]
    for x, y in anchors:
        r = (x * x + y * y) ** 0.5
        start = (x * 34.0 / r, y * 34.0 / r)
        part += arm(start, (x, y), 8.0, -12.0, 3.0)
        part += cyl(6.0, 3.0, x, y, -12.0)
    for x, y, _ in BOARD_POSTS:
        part -= cyl(1.15, 8.0, x, y, -13.0)  # 2.3 mm M2 clearance
    for x, y in anchors:
        part -= radial_slot(x, y)  # 3.6 mm M3 clearance
    return part, anchors


def trim():
    # A simple first-fit matte/paintable annulus sits 0.5 mm in front of the
    # Waveshare face; ID 54.5 mm does not press on the ~53 mm lit circle.
    # It is a placement/appearance gauge, not a finished snap-fit surround.
    return cyl(37.1, 1.2, z=3.9) - cyl(27.25, 2.2, z=3.4)


def place_at_eye(shape, center):
    return Pos(center[0], BOARD_ORIGIN_Y, center[1]) * Rot(90, 0, 0) * shape


def mask_pad(shell, x, z):
    # The rear surface is found along Y at this X/Z; the farthest intersection
    # is the surface a pad placed from behind will meet.
    from build123d import Axis
    def rear_at(px, pz):
        hits = shell.find_intersection_points(Axis((px, -200, pz), (0, 1, 0)))
        return max((point.Y for point, _ in hits), default=None)

    rear_y = rear_at(x, z)
    samples = [rear_y]
    for radius in (3.0, 6.0):
        for step in range(24):
            theta = step * math.tau / 24
            hit = rear_at(x + radius * math.cos(theta),
                          z + radius * math.sin(theta))
            if hit is not None:
                samples.append(hit)
    # Size the *rear* plane from the farthest-back point under the whole pad so
    # its sleeve stays off the mask.
    back_y = max(samples) + 2.0
    front_y = min(samples) - 1.0
    # A small sacrificial pad is cut to the nominal shell surface. Actual
    # print/glue variation needs filler or a foam/epoxy bond after dry fit.
    blank = (Pos(x, back_y, z) * Rot(90, 0, 0)
             * cyl(6.0, back_y - front_y))
    pad = blank - shell
    # 4.2 mm pilot for a test M3 heat-set insert, inserted from behind.
    bore = (Pos(x, back_y + 0.1, z) * Rot(90, 0, 0)
            * cyl(2.1, 4.1))
    pad -= bore
    return pad, rear_y, back_y


pieces = []
for index, (shell, center) in enumerate(zip(HEAD_HALVES, EYE_CENTERS)):
    side = "left" if index == 0 else "right"  # mask STEP negative/positive X
    sign = -1 if index == 0 else 1
    local_carrier, anchors = carrier(sign)
    world_carrier = place_at_eye(local_carrier, center)
    world_trim = place_at_eye(trim(), center)
    export_step(world_carrier, str(OUT / f"{side}_carrier.step"))
    export_stl(local_carrier, str(OUT / f"{side}_carrier.stl"))
    export_step(world_trim, str(OUT / f"{side}_trim_gauge.step"))
    export_stl(trim(), str(OUT / f"{side}_trim_gauge.stl"))
    pieces.extend([world_carrier, world_trim])
    print(f"{side}: carrier {world_carrier.volume:.1f} mm^3, "
          f"trim {world_trim.volume:.1f} mm^3")
    for number, (lx, ly) in enumerate(anchors, 1):
        pad, rear_y, back_y = mask_pad(shell, center[0] + lx,
                                      center[1] + ly)
        export_step(pad, str(OUT / f"{side}_pad_{number}.step"))
        export_stl(pad, str(OUT / f"{side}_pad_{number}.stl"))
        pieces.append(pad)
        gap = -6.0 - back_y  # carrier front to pad rear face
        sleeve_outer_radius = 5.0 if gap > 30.0 else 3.5
        sleeve = (Pos(center[0] + lx, -6.0, center[1] + ly)
                  * Rot(90, 0, 0)
                  * (cyl(sleeve_outer_radius, gap) - cyl(1.8, gap)))
        export_step(sleeve, str(OUT / f"{side}_sleeve_{number}.step"))
        export_stl(cyl(sleeve_outer_radius, gap) - cyl(1.8, gap),
                   str(OUT / f"{side}_sleeve_{number}.stl"))
        pieces.append(sleeve)
        print(f"  pad {number}: X={center[0]+lx:.1f}, Z={center[1]+ly:.1f}, "
              f"rear shell Y={rear_y:.1f}, sleeve={gap:.1f} mm, "
              f"pad volume={pad.volume:.1f} mm^3")

export_step(Compound(children=pieces), str(OUT / "first_fit_mounts_in_mask_coordinates.step"))
export_step(Compound(children=[*HEAD_HALVES, *pieces]),
            str(OUT / "mask_with_first_fit_mounts.step"))
print("Exported test-fit STEP and STL parts to", OUT)
