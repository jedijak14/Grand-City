# Screenshots — what is real and what is a target

## `REALMODEL_*.png` — REAL renders of the included models
These are labeled as renders of included model assets, not in-game screenshots. The offline
preview-generation script mentioned in earlier project notes is not present in this source tree;
these image files are included under `Docs/Screenshots/`.

Current set: buggy, groundvehicle, cesiumair (passenger turboprop — used for the
airliner), babyloncar, soldier, village, milktruck, aerobatic, xbot, fox, horse.

Note: `buggy` is a toy construction model — great for physics testing, but replace
traffic/player cars with high-poly Sketchfab/Fab models (see Docs/MODEL_SOURCES.md).

## `TARGET_*.jpg` — AI-generated visual targets (NOT gameplay)
Design references for how systems should look once assets/lighting are in.
Do not use them for marketing as "screenshots" (one even contains real airline
branding from the generator). Real gameplay screenshots come from the UE5 editor
on the dev PC: HighResShot console command or the Screenshot tool after building.
