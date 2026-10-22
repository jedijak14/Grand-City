# Harbor vertical slice — visual development map

`Tools/BuildHarborVerticalSlice_Editor.py` creates a second, more art-directed play space at
`/Game/Maps/L_HarborVerticalSlice`. It is a **look-development vertical slice**, not finished
AA art and not evidence of an Unreal render. The editor script assembles reusable flat-color PBR
materials, a waterfront boulevard, scored sidewalks, crosswalks, custom mixed-use facades with
recessed glazing, palms, street lights, signals, a boardwalk, an ocean query surface, a player
start, and the native gameplay systems. Its harbor water is a CPU-deformed procedural grid driven
by the same analytic wave queries used by buoyancy, updating at a configurable capped rate. Boat
wakes and low aircraft passes add bounded disturbances to the same rendered and sampled surface;
the material uses vertex color to accent steep crests. It also creates a high-preset scalability
profile for high-end PC presentation.

## Run it in Unreal Editor

1. Build `GrandCityEditor` for UE 5.4 / Win64 and open `GrandCity.uproject`.
2. Run `Tools/ImportModels_Editor.py` first. If a generated Arden GT is available, import it as
   well; the map builder tries it before the sample car meshes for the player vehicle.
3. Use **Tools → Execute Python Script** to run `Tools/BuildHarborVerticalSlice_Editor.py`.
4. Save and open `/Game/Maps/L_HarborVerticalSlice`, then set it as the editor/game startup map.
5. Replace the procedural facade/palm/road details with artist-authored Nanite-ready meshes,
   trim sheets, decals, vegetation, and a matched animated water material before calling it final.

The scene setup adds the existing day/night manager with a UE5-lux-scale sun source, a sky atmosphere,
optional exponential fog, and a restrained post-process volume where the installed UE Python API exposes those settings.
The water surface now deforms to the analytic gameplay wave field rather than relying on a flat
plane or a separate material-only wave approximation. It is still a development implementation:
profile the CPU mesh updates, inspect the normals/material response, and replace it with a scalable
GPU-driven or UE Water implementation if the target-world footprint or frame budget requires it.

## Verification status

The script is syntax-checked only. Its material-graph creation, engine/editor Python API calls,
level appearance, collision, performance, and gameplay must be verified in UE 5.4. The workspace
has no Unreal Editor, no cooked map, and no `Content/` assets, so none of those are claimed as
complete. This is a concrete visual-production foundation, not a promise that an AA-scale game
has already been achieved.
