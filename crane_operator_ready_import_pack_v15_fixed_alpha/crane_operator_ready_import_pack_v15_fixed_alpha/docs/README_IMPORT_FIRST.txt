CRANE OPERATOR READY IMPORT PACK v15 — FIXED ALPHA

Use this pack instead of v14.

Why v15 exists:
The v14 animations looked buggy because some generated sprite sheets had checkerboard backgrounds baked into the image pixels. Codex loaded those pixels as real image data, so the game showed big white/gray checker rectangles behind the animation.

v15 fixes that:
- all frames are reprocessed with transparent backgrounds
- all animations are already sliced into individual PNG frames
- clean sheets are also included
- recommended on-screen sizes are included so Codex does not draw giant sprites
- hitbox rules are included so collision does not use full frame rectangles

Use first:
assets/frames_clean/

Do not use:
old v14 frames
full frame rectangles as hitboxes
native PNG size as the in-game destination size

Recommended integration:
Load individual frames from assets/frames_clean/<asset_id>/<asset_id>_00.png ... _07.png.
Render them into manually tuned destination rectangles using recommended_screen_size from manifests/animation_pack_manifest.json.
