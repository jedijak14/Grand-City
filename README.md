# Grand City — UE5 C++ Open-World Sandbox
(M1: car physics + core sandbox · M2: boat/water sim, plane upgrades, airport computers, passenger flights & voice lines)

A complete C++ gameplay foundation for an open-world sandbox game in **Unreal Engine 5.4**
(should also build on 5.3/5.5 with minor tweaks): custom raycast **car physics** with
drifting and crash deformation, animated NPCs, police & traffic AI, shooting, a flyable
plane, buoyancy boats, parachuting, day/night with real constellations, robbable shops,
a hospital, airports with bookable flights, and a settings-backed world you drive with
free high-poly models you drop in.

> ⚠️ **Build status:** the source tree is now arranged as an Unreal Engine C++ project and
> the missing native UI classes have been added, but this code has not yet been compiled with
> Unreal Engine 5.4 and Visual Studio 2022. No compile, cook, or gameplay result is claimed.
> `.gitattributes` marks model and Unreal binary assets for Git LFS; the existing model blobs
> still need LFS migration before a release-quality source push.

## 1. Build it

Requirements: Windows 10/11, **Unreal Engine 5.4**, **Visual Studio 2022** with
“Game development with C++” + “Desktop development with C++” workloads.

1. Right-click `GrandCity.uproject` → **Generate Visual Studio project files**.
2. Open `GrandCity.sln`, set **Development Editor | Win64**, Build. (Or just double-click
   the `.uproject` and click *Yes* when asked to build.)
3. In the editor: **Edit → Plugins** → enable **Python Editor Script Plugin** (for the
   tools below) and confirm **Procedural Mesh Component** is on. Restart the editor.

## 2. Get a drivable map in one minute

1. File → New Level → **Empty Level**, save it as `Content/Maps/L_TestCity`.
2. Set it as the game map: Project Settings → Maps & Modes → *Game Default Map*.
3. Tools → **Execute Python Script…** → `Tools/BuildStarterCity_Editor.py`.
   You get a grey-box island: road grid, building blocks, runway, ocean, hospital/shop/
   police/airports/marina/elevator, a test car, plane and boat, population spawner.
4. Press **Play**. Walk to the car, press **F**, drive with **WASD**, hold **SPACE** to drift.

The C++ `AGCGameMode` already sets the player pawn, controller, HUD and player state.
The car has fallback engine-shape meshes so it is visible immediately; the character is a
capsule until you assign a skeletal mesh (next step).

For a denser coastal look-development scene, run `Tools/BuildHarborVerticalSlice_Editor.py`.
It creates `/Game/Maps/L_HarborVerticalSlice` with modular storefront facades, glazing, palms,
street furniture, crosswalks, warm lighting, and a player-car spawn. Run the model importer first
for the best available vehicle mesh. Details and the unverified editor workflow are in
`Docs/HARBOR_VERTICAL_SLICE.md`; its procedural architecture remains a placeholder kit, while the harbor surface now uses a configurable CPU-deformed wave grid shared with gameplay queries and stylized boat/low-aircraft wake disturbances. This is an uncompiled, unprofiled development feature—not final AA art. Run the model importer to make the original sunset harbor loading backdrop available to the native loading widget; see `Docs/LOADING_SCREEN_ART.md`.
`Tools/BuildStarterCity_Editor.py` places two moving-tread escalator prototypes at its airport test markers and a moving lobby-to-roof elevator; see `Docs/ESCALATOR_PROTOTYPE.md` and `Docs/ELEVATOR_PROTOTYPE.md`. These are functional blockout systems pending Unreal runtime tests, not finished transport or architectural art. `Config/DefaultScalability.ini` also provides a UE High preset for testing on capable PCs.

## 3. Put real (high-poly) models in

1. **Starter models:** run `Tools/ImportModels_Editor.py` to import the public GLBs in
   `ModelLibrary/PublicGLB/` (sample characters, vehicles, aircraft, animals, and village).
2. **Original vehicle prototype:** run `python -m pip install -r Tools/requirements-models.txt`,
   then `python Tools/CreateGrandCityArdenGT.py` to generate the 176k-triangle Arden GT visual.
   It is a static, unrigged prototype and needs Unreal scale/material/collision review before use.
3. **Sketchfab/Fab models:** download with your free account → drop `.glb/.fbx` into
   `ModelLibrary/SketchfabDropIn/` → run the same importer. Sources & license notes:
   `Docs/MODEL_SOURCES.md`.
4. Create **Blueprint children** and assign meshes:
   - `BP_Car` (parent `GCVehiclePawn`): set **BodyMesh** (enable *Allow CPU Access* on the
     mesh first for crash denting), set **WheelMesh0–3**, tune **Wheels** offsets/radius,
     engine audio/screech sounds, door mesh on `DriverDoorPivot`. Police version: tick
     **bIsPoliceVehicle**.
   - `BP_Player` (parent `GCCharacter`): skeletal **Mesh** + an AnimBP (parent
     `GCAnimInstance`, drive blendspaces from `GroundSpeed/Direction/bAiming/bSeated/...`),
     parachute canopy mesh, optional Chaos-cloth hoodie/hair.
   - `BP_Ped` (parent `GCPedestrian`), `BP_Plane`, `BP_Boat` similarly.
5. Point the game at your BPs: World Settings → GameMode Override → a BP child of
   `GCGameMode` with your pawn classes, and set the class slots on `GCWorldSpawner`.

Loading-screen artwork setup and the original gameplay-look mockup are documented in
`Docs/LOADING_SCREEN_ART.md`. The mockup is concept art, not an Unreal gameplay capture.

## 4. Controls

| Context | Keys |
|---|---|
| On foot | WASD move · Mouse look · Shift sprint · Space jump · RMB aim · LMB fire · R reload · **1–4** weapons · **F** enter vehicle · **E** use location · **P** phone · **X** parachute · **V** first/third person |
| Car | W/S throttle/brake/reverse · A/D steer · **Space** handbrake/drift · Mouse orbit · **V** chase/cockpit · L lights · H siren (police) · **F** exit |
| Plane (yours) | Shift/Ctrl throttle · ↑/↓ or Mouse pitch · A/D roll · Q/E rudder · **F** flaps · **G** gear · **B** spoilers · **Space** wheel brakes · **R** refuel on ground · **E** exit (then **X** parachute) |
| Boat | W/S throttle · A/D rudder · **Space** anchor · F exit |
| Airport computer | **Tab** choose flight · **B** choose class · **E** book, then **E** again to check in; selection and action feedback animate on the kiosk overlay |
| Phone | **P** open/close the animated wallet and boarding-pass screen |
| Airliner seat | **1** flight map · **2** movies · **3** music · **4** games · **0** screen off · **V** camera · **E** disembark at gate |
Dev console (**~**): `GCSpawnCar` · `GCSetWanted 4` · `GCGiveMoney 5000` · `GCSetTime 22.5`
· `GCTeleport 32000 0 500` · `GCRepairVehicle` · `GCGod` · `GCSetJob President`

## 5. What's implemented (code map)

- `Vehicle/GCVehiclePawn` — suspension/tyre/gearbox physics, drift, skidmark + deformation
  hookup, door/seat enter-exit, police lights/siren, engine & screech audio
- `Components/GCDeformationComponent` — permanent crash denting of the body mesh
- `Components/GCSkidmarkComponent`, `GCWeaponComponent`, `GCHealthComponent` (blood FX
  respects the settings toggle), `GCParachuteComponent`
- `Character/GCCharacter` + `GCAnimInstance`, `GCPedestrian`
- `AI/*` — ped wander/flee/police (arrest vs lethal by stars), traffic & pursuit drivers
- `World/*` — day/night + real star data, shared ocean wave field, locations, population spawner
- `Vehicle/GCPlanePawn` (lift/stall/flaps/gear/spoilers/ground effect/fuel/wheel brakes),
  `Vehicle/GCBoatPawn` (multi-point buoyancy on a moving wave field, currents, planing lift,
  anchor, wake/spray FX hooks), `World/GCOceanManager` (height/normal/velocity/current/foam)
- **Airports:** `World/GCComputerTerminalActor` (booking + check-in computers, schedules,
  seat classes, boarding passes in the player wallet) and `Vehicle/GCAirlinerPawn`
  (passenger flights: announcements, NPC passengers, seatback entertainment)
- `Components/GCVoiceLineComponent` — check-in staff, cabin crew, captain and NPC voice
  lines with subtitles (script + wiring in `Docs/VOICE_LINES.md`, audio in `AudioVoice/`)
- `Core/*` + `UI/GCHUD` — game mode, wanted/money, settings (`GCGameUserSettings`: blood,
  waves, grass, cloth/hair, densities, day length), native money/wanted/health/speed HUD, dev commands
- **Phase 2:** `Vehicle/GCSurfboardPawn` (surfboard/paddleboard/raft on the wave field),
  `World/GCGrassFieldActor` (HISM grass, wind phase, settings-gated), `World/GCMarineLifeActor`
  (fish school + dolphins), native UMG menus/loading, animated airport flight-selection panel,
  slide-in wallet/boarding-pass phone, and elevator floor-button feedback; crouch (C), expanded
  `GCAnimInstance` (see `Docs/ANIMATION_SETUP.md`), `AudioSFX/` + `Docs/SFX.md`, voice batch 2

See `ROADMAP.md` for what remains (full phone apps, AnimBP/Niagara editor assets, packaged-game
build + verified updater, then multiplayer) and `Docs/PACKAGING_AND_WEBSITE.md` for the live
website + UE5-project installer details.

**Minimum PC to develop:** as for any UE5 project — Win10/11 64-bit, 6-core CPU, 16 GB RAM,
DX12 GPU with 6 GB+ VRAM, ~40 GB disk for engine + project.
