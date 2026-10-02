# Freddy eye appearance baseline

Established on 2026-09-21 from Joshua's supplied [`official_freddy_eye_texture.webp`](../reference_images/official_freddy_eye_texture.webp). Both boards show the texture moving and blinking correctly; Joshua confirmed their matching brown eyelids and perimeter rings look good. This is the current visual baseline, not a camera-tracking test.

- **Iris and pupil:** Use the supplied blue radial texture, including its black pupil and two white highlights. The original file remains unchanged. The embedded iris is 178 × 178 pixels with a transparent feathered edge.
- **Sclera:** Warm cream `#F5F4EA`, 432-pixel circle on the 480 × 480 canvas.
- **Outer eye ring:** Brown `#743824`, matching the lids; 444-pixel circle.
- **Lids:** Brown `#743824`, chosen by Joshua after the first textured-eye test. The current blink is a straight upper/lower closure lasting 300 ms every 4.6 seconds; this eyelid shape can be refined for the final character expression.
- **Surrounding background:** Dark brown `#39271D`.
- **Brightness:** The Waveshare driver initializes its backlight to 50 on its 0–100 setting. This is the tested bench setting, not a measured luminance value.

[Display composition preview](../firmware/assets/freddy_eye_display_preview.png) shows the centered open-eye layout without the small board-test label. The [iris-only preview](../firmware/assets/freddy_iris_preview.png) shows the encoded image and transparent edge. The physical display was visually confirmed by Joshua; a screen photo and frame-rate measurement remain to be recorded.

[Mid-blink preview](../firmware/assets/freddy_eye_blink_preview.png) shows the new eyelid color. The straight-edge lids are an early animation baseline rather than sculpted eyelids.
