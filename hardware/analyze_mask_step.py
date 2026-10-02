"""Measure straight-on clear eye apertures in the Shapr3D mask export.

Run with: uv run --no-project --with build123d python hardware/analyze_mask_step.py
This is read-only. A 0.5 mm sightline grid gives approximate, not print-fit,
measurements; faceted CAD and the assembled print may differ.
"""

from collections import deque
from pathlib import Path

from build123d import Axis, import_step
from scipy.ndimage import distance_transform_edt
import numpy as np


STEP = Path(__file__).resolve().parents[1] / "stls/freddy_fazbear_mask.step"
GRID_MM = 0.5
mask = import_step(str(STEP))
solids = mask.solids()


def clear_aperture(solid_index: int, x_start: int, seed_x: int) -> None:
    """Flood-fill only the enclosed eye opening, not empty space off-mask."""
    solid = solids[solid_index]
    xs = [x_start + i * GRID_MM for i in range(241)]
    zs = [160 + i * GRID_MM for i in range(141)]
    open_cells = set()
    for zi, z in enumerate(zs):
        for xi, x in enumerate(xs):
            axis = Axis((x, -200, z), (0, 1, 0))
            if not solid.find_intersection_points(axis):
                open_cells.add((xi, zi))

    seed = (round((seed_x - x_start) / GRID_MM), round((195 - 160) / GRID_MM))
    assert seed in open_cells, f"Expected opening seed is not clear: {seed}"
    component = {seed}
    queue = deque([seed])
    while queue:
        xi, zi = queue.popleft()
        for neighbor in ((xi - 1, zi), (xi + 1, zi), (xi, zi - 1), (xi, zi + 1)):
            if neighbor in open_cells and neighbor not in component:
                component.add(neighbor)
                queue.append(neighbor)
    assert all(0 < xi < len(xs) - 1 and 0 < zi < len(zs) - 1 for xi, zi in component), (
        "The eye component reached the scan boundary; expand the grid or inspect the model."
    )

    x_coords = [xs[xi] for xi, _ in component]
    z_coords = [zs[zi] for _, zi in component]
    rows = {}
    columns = {}
    for xi, zi in component:
        rows.setdefault(zi, []).append(xi)
        columns.setdefault(xi, []).append(zi)
    widest_row = max(rows, key=lambda row: max(rows[row]) - min(rows[row]))
    tallest_column = max(columns, key=lambda col: max(columns[col]) - min(columns[col]))
    print(f"Eye half {solid_index}: {len(component)} clear samples, grid {GRID_MM} mm")
    print(f"  projected clear bounds: X {min(x_coords):.1f}..{max(x_coords):.1f} mm, "
          f"Z {min(z_coords):.1f}..{max(z_coords):.1f} mm")
    print(f"  max horizontal clear width: "
          f"{(max(rows[widest_row]) - min(rows[widest_row])) * GRID_MM:.1f} mm "
          f"at Z={zs[widest_row]:.1f} mm")
    print(f"  max vertical clear height: "
          f"{(max(columns[tallest_column]) - min(columns[tallest_column])) * GRID_MM:.1f} mm "
          f"at X={xs[tallest_column]:.1f} mm")
    print(f"  clear-area centroid: X={sum(x_coords)/len(x_coords):.1f}, "
          f"Z={sum(z_coords)/len(z_coords):.1f} mm")
    aperture = np.zeros((len(zs), len(xs)), dtype=bool)
    for xi, zi in component:
        aperture[zi, xi] = True
    distances = distance_transform_edt(aperture) * GRID_MM
    center_zi, center_xi = np.unravel_index(np.argmax(distances), distances.shape)
    print(f"  largest straight-on inscribed circle: radius about "
          f"{distances[center_zi, center_xi]:.1f} mm at "
          f"X={xs[center_xi]:.1f}, Z={zs[center_zi]:.1f} mm")


clear_aperture(0, -110, -30)
clear_aperture(1, 15, 60)
