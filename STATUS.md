# Grand City — project status / handoff

## Verified in this workspace

- The current project tree is being normalized to an Unreal Engine C++ layout: module code in
  `Source/GrandCity/`, targets in `Source/`, engine INI files in `Config/`, and external starter
  assets in `ModelLibrary/`, `AudioVoice/`, and `AudioSFX/`.
- Native UI source files now exist for the HUD, main menu, settings, and loading screen. The
  loading overlay now includes a self-contained skyline, branded streaming card, tip, and
  progress percentage. `Docs/LoadingScreenConcept.png` is a visual concept, not an Unreal capture.
- Ocean queries now provide analytic wave normals, vertical and horizontal orbital velocities,
  and boats, swimmers, and boards use the nearest ocean manager's world height/amplitude.
- The `.uproject` parses as valid JSON and targets Unreal Engine 5.4. ProceduralMeshComponent
  and Water remain enabled; ChaosVehicles remains deliberately disabled.
- The current code inventory is 32 `.cpp` files and 34 headers after adding the missing UI
  module files. The uploaded tree has 15 public GLBs, 20 voice WAVs, and 17 SFX WAVs. A new
  176k-triangle original vehicle prototype GLB is generated locally by `Tools/CreateGrandCityArdenGT.py`; it is not yet committed through Git LFS.
- `.gitignore` excludes Unreal-generated/build output. `.gitattributes` marks model and Unreal
  binary assets for Git LFS.

## Not yet verified / blockers

- `Tools/BuildHarborVerticalSlice_Editor.py` and `Config/DefaultScalability.ini` now define an art-directed coastal look-development pass. The editor script is syntax-checked only; the generated map/materials, lighting, collisions, performance, and on-screen result have not been tested.
- **No Unreal Engine 5.4 or Visual Studio 2022 compiler is available in this workspace.** C++
  still needs an actual Development Editor | Win64 build on the owner's Windows PC. No compile,
  cook, editor launch, or gameplay test is claimed.
- Git LFS is not installed in this workspace. The new attributes are present, but the existing
  model blobs have not yet been migrated into LFS storage. Install Git LFS before the next
  push/clone that relies on those assets, then migrate/re-upload if required.
- Editor helpers for model/audio import, greybox map creation, and AnimBlueprint shells have been
  added under `Tools/`; they pass Python syntax checks but have not been run inside Unreal Editor.
  The NSIS installer source, SFX generator, and offline preview renderer referenced in older
  notes are still absent from the uploaded source tree.
- Unreal editor-only assets (maps, Blueprints, AnimBP graphs, Niagara systems) are not included.

## Next verification gate

On Windows with UE 5.4 + VS2022: install Git LFS, pull the project, right-click
`GrandCity.uproject` → Generate Visual Studio project files, build **Development Editor | Win64**,
then send the first compiler errors verbatim. After a successful build, open the editor and
verify the starter-map workflow before claiming playability.

## Honest limits

This is a C++ prototype, not a standalone packaged game. The project installer (if restored and
verified) installs the UE project and still requires UE5 + VS2022. Crash deformation is mesh
denting rather than true soft-body physics. Art-target images are labeled `TARGET_*`; they are
not gameplay screenshots.
