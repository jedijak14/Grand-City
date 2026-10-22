# Grand City — project status / handoff

## Current implementation

- Unreal Engine 5.4 C++ project source lives under `Source/GrandCity/`; generated maps and imported
  content are editor outputs and are not checked in.
- Native loading UI draws the streaming card/progress display and can resolve the original sunset
  harbor backdrop from `/Game/GrandCity/UI/LoadingScreenBackdrop` after the model importer runs.
  The concept images remain mockups, not Unreal screenshots.
- `AGCOceanManager` provides analytic wave height/normal/velocity queries and an optional
  CPU-deformed procedural surface. Boat wake packets and fast, low plane passes contribute a
  bounded stylized wake height, slope, and velocity to physics samples and the harbor mesh.
- `AGCEscalatorActor` circulates instanced treads and gives grounded characters upward movement in
  its ride zone. The starter-map script places two blockout escalators beside airport test markers.
- `AGCElevatorActor` now runs a one-car lobby-to-roof cycle with a boarding dwell, animated split
  doors, collision walls, a moving floor, and two linked interaction landings in the starter map.
- The harbor look-development builder adds a coastal boulevard and procedural facades/props,
  player spawn, lighting setup, a 192-cell wave surface, and a high scalability profile.
- `Tools/CreateGrandCityArdenGT.py` generates a 176k-triangle original static vehicle prototype.
  The available GLB character/vehicle models are samples; production-quality custom, rigged car
  and character assets are still outstanding.

## Verification status and blockers

- Python syntax/AST checks and `git diff --check` have passed for the editor scripts and latest
  source changes. These do not validate Unreal APIs or runtime behavior.
- **No Unreal Engine 5.4 or Visual Studio 2022 compiler is available in this workspace.** The new
  C++ additions (water wake response, escalator, and elevator actors), editor material creation, map
  appearance, moving-platform carry, landing overlap, collision, animation, cooking, and performance
  have not been built or run in Unreal.
- Water/wake behavior is a stylized empirical model, not a full fluid solver. The CPU surface and
  wake packet count require profiling at the actual target resolution and actor density.
- The starter map remains greybox. Bathrooms, authored terminal interiors, production escalator
  rails/treads, original high-quality vehicle/character models, rigged animation sets, and final
  lighting/material review are incomplete.
- Git LFS is not installed in this workspace. Do not add large model binaries as ordinary Git blobs;
  install and use LFS before publishing generated/imported model assets.
- The loading image import is automated, but a packaged build still needs an Unreal cook/runtime
  check. Niagara and Animation Blueprint graphs still require editor setup and verification.

## Next verification gate

On Windows with UE 5.4 + Visual Studio 2022: install Git LFS, build **Development Editor | Win64**,
run `Tools/ImportModels_Editor.py`, then build and Play the starter map. Check escalator collisions
and rider motion, verify the loaded backdrop, test boat/low-plane wakes and buoyancy, and profile
the harbor surface with multiple wake sources. Fix compiler/runtime errors before presenting this
as a playable or AA-quality result.
