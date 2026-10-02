# Eye mounts — first-fit CAD, not final print approval

This is a removable **board-carrier prototype** for the stationary Freddy mask.
It is built from the mask STEP and the official Waveshare
ESP32-S3-Touch-LCD-2.1 assembly. Coordinates are millimetres in the mask STEP.
`left` means the negative-X eye in that STEP; check the physical orientation
before printing. The unmounted circuit boards, plugged USB/UART cables, glued
mask seams, and print tolerances are **not** represented by the exported mask
assembly.

## What is here

- `mask_with_first_fit_mounts.step`: two mask head halves plus all mount parts,
  for inspection in Shapr3D. Import as millimetres. The mask is reference
  geometry, not part of the print job.
- `first_fit_mounts_in_mask_coordinates.step`: mount parts only, in their
  candidate installed positions.
- `left_carrier` / `right_carrier`: rear ring with four 2.3 mm nominal M2
  clearance holes matching the vendor board's asymmetric rear posts. Three
  3.6 mm wide radial slots allow about 5 mm of in-plane placement adjustment.
- `*_pad_1..3`: small shell-conforming feet. The two upper feet use the brow;
  the third uses the lower *outer* cheek rather than the steep temple or the
  glued center seam. The 4.2 mm, 4 mm-deep blind bores are only **test pilots**
  for a specific M3 heat-set insert. Confirm the insert's actual specification
  and the printed wall thickness before installation.
- `*_sleeve_1..3`: stand-off sleeves between carrier and pads. Approximate
  lengths are 2.0, 12.2–12.3, and 47.0 mm. The lower sleeve is 10 mm OD and
  needs a straight M3 fastener/rod solution that is measured on the real mask.
- `*_trim_gauge`: 74.2 mm OD, 54.5 mm ID, 1.2 mm thick visual gauge to test
  how a mask-colored annulus looks around a powered eye. It is **not** a
  finished, retained cosmetic surround and must not touch the display glass.

The board centers in this candidate pose are X/Z = (-30, 196) and (56, 196),
with board origin Y=-15 and display pointing toward negative Y. That depth
cleared the vendor bare assembly in CAD; it is **not** a guarantee that the
actual plugged USB and UART cables, connector bends, indicator LED, or user's
Rev2.0 board will clear. The lower cheek pad makes this test layout sturdier
than a third pad on the sharply sloped side surface, but the 47 mm spacer and
mask-side adhesion need a physical load/fit test.

## Print and dry-fit sequence

1. Print **one eye** first: one carrier, pads 1–3, sleeves 1–3, and optionally
   the trim gauge, using the matching `.stl` files. Do not print the mask STEP.
2. Compare the printed carrier's four holes with the actual board posts. Use
   nonconductive hardware; choose screw lengths from the physical post depth,
   not from CAD alone. Do not drive a screw into the PCB or display.
3. Hold the board/carrier behind the mask with the real USB and 4-pin UART
   plugs fitted. Check both ports, wire bend radius, strain relief, and glass
   clearance. The separate UART USB-C port remains unplugged for header RX.
4. Tape the three pads in place **temporarily**. Check whether their curved
   faces match the glued print, whether the carrier slots align, and whether
   the sleeves fit without loading or bending the board. Do not glue, drill,
   heat-set, or force the mask during the first dry fit.
5. Power both eyes and inspect from normal visitor distance and close-up.
   Check edge clipping, iris symmetry, dark-cavity visibility, and the red
   onboard indicator. Use the trim gauge only to assess color/reveal; a true
   integrated tapered surround still needs to be shaped and retained after
   this fit check.

If the pads are eventually bonded to the rear shell, the board/carrier can be
removed by undoing its fasteners. The adhesive method and final fastener
stack are intentionally undecided until the physical dry fit. The carrier is
not intended to be bonded directly to the PCB or to the glued center seam.

Regenerate parts with:

```sh
uv run --no-project --with build123d python hardware/eye_mount/first_fit.py
```

For a repeatable check against a downloaded copy of the official Waveshare
board STEP, run `check_vendor_clearance.py /path/to/board.step` with the same
`uv run --no-project --with build123d python` prefix. The vendor file is not
committed here; its source is the [Waveshare wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.1).
