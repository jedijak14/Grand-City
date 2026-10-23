# Gridlock whole-library art quality plan

## Goal

The whole shipped experience—not just a hero building—must read as a coherent, high-quality modern city. Existing Kenney/stylized and other low-detail assets are not final merely because they are licensed or imported. The existing density report is only a triage signal; triangle count is neither a visual-quality score nor a production pass.

## Hard constraint and current state

The repository environment permits outbound access only to a small allowlist of hosts, which does not include Sketchfab's model pages, Download API, or signed binary-download hosts. The Sketchfab connector is unsupported here. Consequently, searching listing metadata is possible via prior research, but authenticating, downloading, visually inspecting, and legally validating the actual files is not. No Sketchfab model has been adopted. Do not work around this by scraping, bypassing access controls, or substituting unrelated sources for the user's explicit Sketchfab requirement.

To unblock actual production work, the project owner needs to provide downloaded, permitted source archives/models. Upload a ZIP or the original model files under `ModelLibrary/SketchfabDropIn/`, together with each model page URL and its license/attribution. Prefer batches by category, starting with a cohesive city/building kit. Do not include credentials or API keys. The key previously shared in chat should be revoked/rotated; it is not needed for this upload workflow.

## Replacement order

Work in cohesive, visible batches; each batch must be reviewed before advancing:

1. **Buildings and streetscape** — coherent architectural families, facades, storefronts, roofs, doors/windows, sidewalks and street furniture; replace the low-quality downtown/background silhouette assets together so no conspicuous old kit remains.
2. **Roads and vehicles** — road surface/markings/curbs and a matching traffic fleet, then emergency vehicles and aircraft/airport assets. Use unbranded or properly licensed liveries.
3. **Interiors and transit** — airport interiors, escalator step/rail assemblies, elevator guides/cab/doors, jetway and ticketing counters. Small mechanical details should be modeled where visible and functional, not represented by bare blocks.
4. **Characters and clothing** — consistent human proportions, facial/material quality, properly rigged clothing, police uniforms and civilian variety.
5. **Props and environment** — signs, lights, barriers, kiosks, landscaping, interiors, handheld objects and remaining interactive props.

## Per-asset acceptance checklist

For every candidate, record its exact source URL, author, license, required attribution, and any restrictions. Inspect the actual downloaded files and representative close/mid/far views. Reject if it has unsuitable art direction, poor silhouette/surface detail, broken or overlapping geometry, bad normals/UVs, missing or weak textures/material maps, unlicensed marks, or an implausible scale/origin. Check scene hierarchy and moving parts where relevant. Assess performance in context: mesh/material count, texture memory, instancing, sensible LODs and collision—not raw triangles alone. Then import into Unreal Engine 5.8, inspect materials and scale, place in the actual scene, check collision/animation, and profile representative gameplay views. Keep the previous asset until its replacement passes.

## Batch review protocol

For each uploaded batch: inventory and hash source files; inspect archive safety; extract without overwriting existing assets; produce per-asset metadata and image previews; review licenses and visual fit; record accepted/rejected/deferred decisions; optimize only from authorized sources; prepare an attribution record; integrate accepted assets behind reversible references; and validate in UE 5.8. Never mark a batch complete based solely on an automated report or CPU preview. If UE is not available, mark engine validation explicitly pending and request a UE screenshot/log from the project owner.

## First requested upload

Please upload one permitted Sketchfab building/architecture batch (GLB/FBX/OBJ or original ZIP, including textures) into `ModelLibrary/SketchfabDropIn/`. Include the source page URLs and exact license/credit text in a `CREDITS.txt`. The previously researched 38-building “Terminal” kit is only a lead, not an approved asset; only upload it if the page still grants you download/use rights and its license suits Gridlock. After the building batch is assessed, proceed category by category rather than scattering one-off assets across the project.
