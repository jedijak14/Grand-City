# Grand City — project status / handoff

## Current implementation

- Unreal Engine 5.8 C++ project source lives under `Source/GrandCity/`; generated maps and imported
  content are editor outputs and are not checked in.
- Native loading UI draws the streaming card/progress display and can resolve the original sunset
  harbor backdrop from `/Game/GrandCity/UI/LoadingScreenBackdrop` after the model importer runs.
  The concept images remain mockups, not Unreal screenshots.
- `AGCOceanManager` provides analytic wave height/normal/velocity queries and an optional
  CPU-deformed procedural surface. Boat wake packets and fast, low plane passes contribute a
  bounded stylized wake height, slope, and velocity to physics samples and the harbor mesh.
- `AGCEscalatorActor` circulates instanced GLB treads and gives grounded characters upward movement
  in its ride zone. The starter map adds detailed static glass side guards/steel handrails; a modeled
  guide frame with structural rails, cross-bracing and an access ladder surrounds the elevator.
- `AGCElevatorActor` now runs a one-car lobby-to-roof cycle with caller-floor pickup, a boarding dwell,
  animated split doors, a doorway obstruction sensor, collision walls, a moving floor, and two linked
  interaction landings in the starter map.
- The starter-city builder currently references original Coastline airport terminal GLBs,
  reciprocal ticketed airliner pawns, a procedural animated ocean, Kenney CC0 buildings/roads/props,
  and custom transit GLBs. One original Pacific Residences high-rise (195,248 triangles with a
  generated PBR atlas) now replaces a simple community skyscraper as the downtown landmark; the
  builder adds a hidden simple collision proxy. This is a first hero-asset replacement, not a
  completed city art pass: 103 of 106 GLBs still fail the report-only density screening floors.
  Low-density assets are not accepted as final production art. `Docs/ModelGallery/index.html` links
  all 106 individual offline CPU previews; these are not UE screenshots. Unreal 5.8 import, art
  review, and runtime verification remain outstanding.
- `Tools/CreateGrandCityArdenGT.py` generates a 176k-triangle original static vehicle prototype.
  The available GLB character/vehicle models are samples; production-quality custom, rigged car
  and character assets are still outstanding. Six Quaternius CC0 rigged character/outfit GLBs
  were added under `ModelLibrary/Clothing/Quaternius/` (casual, formal, swimwear, tactical
  reference), converted to standard GLB, and included in the editor import script. Unreal import,
  retargeting, outfit-preset wiring, and cloth simulation remain outstanding.

## Verification status and blockers

- Python syntax/AST checks, the offline Quaternius wardrobe structural validator, and
  `git diff --check` pass. They do not validate Unreal C++ APIs or runtime behavior. The new
  `UGCClothPhysicsLibrary` toggles built-in Chaos Cloth controls across player/NPC skeletal meshes,
  but requires authored Clothing Data and still needs UE 5.8 compilation and simulation testing.
- **No Unreal Engine 5.8 or Visual Studio 2022 compiler is available in this workspace.** The new
  C++ additions (water wake response, escalator, and elevator actors), editor material creation, map
  appearance, moving-platform carry, landing overlap, collision, animation, cooking, and performance
  have not been built or run in Unreal.
- Water/wake behavior is a stylized empirical model, not a full fluid solver. The CPU surface and
  wake packet count require profiling at the actual target resolution and actor density.
- City buildings, airport shells, road slabs, POI buildings, and transit guide/rail/tread geometry
  no longer use visible cube stand-ins in the starter-map builder. The editor has not been run here,
  so GLB import, Kenney texture resolution, actual mesh bounds/orientation, collision, ocean/terrain
  appearance and overlap are all unverified. Minor interactive props and native vehicle actors still
  use simple built-in meshes; this pass does not complete all production vehicle art, interiors,
  animations, Niagara effects, or final art direction.
- Git LFS is not installed in this workspace. Do not add large model binaries as ordinary Git blobs;
  install and use LFS before publishing generated/imported model assets.
- The loading image import is automated, but a packaged build still needs an Unreal cook/runtime
  check. Niagara and Animation Blueprint graphs still require editor setup and verification.

## Next verification gate

On Windows with UE 5.8 + Visual Studio 2022: install Git LFS, build **Development Editor | Win64**,
run `Tools/ImportModels_Editor.py`, then build and Play the starter map. Check escalator collisions
and rider motion, verify the loaded backdrop, test boat/low-plane wakes and buoyancy, and profile
the harbor surface with multiple wake sources. Fix compiler/runtime errors before presenting this
as a playable or AA-quality result.
