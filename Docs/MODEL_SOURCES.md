# Model sources — what's included & where to get high-poly free models

## Included starter library (`ModelLibrary/PublicGLB/`, 15 files, ~33 MB)

Downloaded from public sample repositories for prototyping. **Re-check each license
before any commercial/paid release** — sample-repo models are fine for development,
not automatically for shipping a paid game.

| File | Source |
|---|---|
| `soldier.glb`, `xbot.glb` | three.js r128 examples (`github.com/mrdoob/three.js`) — animated humanoid characters (idle/walk/run) |
| `cesiumman.glb`, `brainstem.glb`, `buggy.glb`, `milktruck.glb`, `fox.glb`, `duck.glb` | Khronos glTF Sample Models (`github.com/KhronosGroup/glTF-Sample-Models`) |
| `astronaut.glb`, `horse.glb` | modelviewer.dev shared assets |
| `groundvehicle.glb`, `cesiumair.glb` | CesiumGS/cesium SampleData |
| `babyloncar.glb`, `aerobatic.glb`, `village.glb` | Babylon.js public assets (`assets.babylonjs.com`) |

Use `soldier`/`xbot` first for the player & pedestrians, `buggy`/`groundvehicle`/`milktruck`
for traffic variety, `cesiumair`/`aerobatic` for planes, `fox`/`horse`/`duck` for wildlife.

## High-poly / high-detail free sources (your account needed where noted)

Sketchfab and Fab downloads are **login-gated** — no tool can pull them without your
account. Workflow: download (free, check the CC license on the page) → drop the
`.glb`/`.fbx` into `ModelLibrary/SketchfabDropIn/` → run `Tools/ImportModels_Editor.py`
→ assign the mesh on a Blueprint child of the matching C++ pawn.

1. **Sketchfab** (filter: Downloadable + Free): search terms that work well —
   "sedan interior glb", "police car", "sports car PBR", "low poly city props" for props,
   "airplane PBR", "speed boat", "rigged character". Prefer CC-BY/CC0, keep the author
   credit in `CREDITS.txt` in the same folder.
2. **Fab (Epic)** — free section: Megascans props, "City Sample" vehicles/buildings
   (large downloads, requires Epic account; vehicles there are skeletal — fine, our
   car physics also accepts them as BodyMesh if you pose them as static).
3. **Mixamo** (free Adobe account) — rigged characters + thousands of animations
   (walk/run/aim/driving-seated) that retarget onto the characters above.
4. **Poly Haven** (polyhaven.com, CC0, no login) — photoreal props + 4K/8K PBR textures.
5. **ambientCG** (CC0, no login) — 4K road/asphalt/concrete/grass textures for the city.
6. **Quaternius / Kenney** (CC0) — stylized low-poly; only use if you accept that art style.

## "Based on real cities" options

- **OpenStreetMap → Blender (BlenderGIS / OSM plugins)** → export FBX → import here:
  gives a real city's road layout + building footprints (heights estimated).
- **Cesium for Unreal + Google Photorealistic 3D Tiles**: real-world textured cities,
  needs API keys and streams data online — great for flying over a real city, heavy for gameplay.
- Practical route for this project: grey-box layout first (Tools script), then dress
  districts with imported models district by district.
