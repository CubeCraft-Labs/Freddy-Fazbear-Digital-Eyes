"""Encode the supplied Freddy iris as a 178 px LVGL RGB565+alpha image.

The source WebP stays unchanged. Run from the project root with the bundled
Python (Pillow required): python firmware/tools/make_iris.py
"""

from pathlib import Path
from PIL import Image, ImageDraw
import hashlib
import math


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "reference_images/official_freddy_eye_texture.webp"
OUTPUT = ROOT / "firmware/eye_test/FreddyIris.cpp"
PREVIEW = ROOT / "firmware/assets/freddy_iris_preview.png"
DISPLAY_PREVIEW = ROOT / "firmware/assets/freddy_eye_display_preview.png"
BLINK_PREVIEW = ROOT / "firmware/assets/freddy_eye_blink_preview.png"
RED_PREVIEW = ROOT / "firmware/assets/freddy_eye_red_preview.png"
SIZE = 178


def main() -> None:
    source = Image.open(SOURCE).convert("RGB")
    if source.size != (508, 466):
        raise ValueError(f"Unexpected source size: {source.size}")

    # The artwork's painted disc spans roughly x=159..372, y=126..337.
    # Keep an equal square around that disc, then scale to the renderer's iris.
    iris = source.crop((159, 125, 373, 339)).resize(
        (SIZE, SIZE), Image.Resampling.LANCZOS
    )
    preview = Image.new("RGBA", (SIZE, SIZE))
    encoded = bytearray()
    radius = SIZE / 2

    for y in range(SIZE):
        for x in range(SIZE):
            r, g, b = iris.getpixel((x, y))
            distance = math.hypot(x + 0.5 - radius, y + 0.5 - radius)
            # Two-pixel feather removes the source's white rectangular backing.
            alpha = round(max(0.0, min(1.0, (radius - distance) / 2.0)) * 255)
            preview.putpixel((x, y), (r, g, b, alpha))
            rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            encoded.extend((rgb565 & 0xFF, rgb565 >> 8, alpha))

    PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    preview.save(PREVIEW)
    display = Image.new("RGBA", (480, 480), (57, 39, 29, 255))
    draw = ImageDraw.Draw(display)
    draw.ellipse((18, 18, 461, 461), fill=(116, 56, 36, 255))
    draw.ellipse((24, 24, 455, 455), fill=(245, 244, 234, 255))
    display.alpha_composite(preview, (151, 151))
    DISPLAY_PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    display.save(DISPLAY_PREVIEW)
    blink = display.copy()
    blink_draw = ImageDraw.Draw(blink)
    blink_draw.rectangle((0, 0, 479, 139), fill=(116, 56, 36, 255))
    blink_draw.rectangle((0, 340, 479, 479), fill=(116, 56, 36, 255))
    blink.save(BLINK_PREVIEW)
    red = Image.new("RGBA", (480, 480), (57, 39, 29, 255))
    red_draw = ImageDraw.Draw(red)
    red_draw.ellipse((18, 18, 461, 461), fill=(116, 56, 36, 255))
    red_draw.ellipse((24, 24, 455, 455), fill=(33, 25, 27, 255))
    for diameter, color, alpha in (
        (178, (21, 9, 9), 255),
        (144, (110, 11, 8), 34),
        (116, (190, 27, 19), 55),
        (82, (226, 45, 30), 90),
        (48, (250, 65, 43), 165),
        (28, (255, 58, 43), 255),
        (13, (255, 242, 227), 255),
    ):
        layer = Image.new("RGBA", red.size)
        offset = (480 - diameter) // 2
        ImageDraw.Draw(layer).ellipse(
            (offset, offset, offset + diameter - 1, offset + diameter - 1),
            fill=(*color, alpha),
        )
        red = Image.alpha_composite(red, layer)
    red.save(RED_PREVIEW)
    lines = [
        "// Generated from reference_images/official_freddy_eye_texture.webp.",
        f"// Source SHA-256: {hashlib.sha256(SOURCE.read_bytes()).hexdigest()}",
        "// Regenerate with firmware/tools/make_iris.py; do not edit this array.",
        '#include "FreddyIris.h"',
        "",
        "static const uint8_t kFreddyIrisPixels[] = {",
    ]
    for offset in range(0, len(encoded), 18):
        lines.append(
            "  " + ", ".join(f"0x{byte:02x}" for byte in encoded[offset : offset + 18]) + ","
        )
    lines += [
        "};",
        "",
        "const lv_img_dsc_t kFreddyIris = {",
        "  .header = {.cf = LV_IMG_CF_TRUE_COLOR_ALPHA, .always_zero = 0,",
        f"             .reserved = 0, .w = {SIZE}, .h = {SIZE}}},",
        "  .data_size = sizeof(kFreddyIrisPixels),",
        "  .data = kFreddyIrisPixels,",
        "};",
    ]
    OUTPUT.write_text("\n".join(lines) + "\n")
    print(f"Wrote {OUTPUT} ({len(encoded):,} image bytes) and previews in {PREVIEW.parent}")


if __name__ == "__main__":
    main()
