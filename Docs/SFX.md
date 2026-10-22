# Sound effects — files & wiring

The repository contains 17 original synthesized SFX WAV files (no ripped game sounds) in
`AudioSFX/`. The generator script mentioned in earlier project notes is not present in this
source upload. Run `Tools/ImportModels_Editor.py` to import the WAVs to `/Game/Audio/SFX`,
then assign them as below. Loops are crossfaded so
they loop cleanly — set **Looping** on the Sound Wave (or use a Sound Cue with a
looping node) for every file marked *loop*.

## Vehicles

| File | Where to assign | Notes |
|---|---|---|
| `engine_loop.wav` *(loop)* | `BP_Car` (`GCVehiclePawn`) → **EngineAudio** component → Audio → Sound | C++ drives pitch/volume from RPM in `UpdateAudioAndLights`. |
| `tyre_screech.wav` | `BP_Car` → **ScreechAudio** → Sound | C++ fades it in from tyre slip/drift. Set Looping for long drifts. |
| `siren_loop.wav` *(loop)* | Police `BP_Car` (`bIsPoliceVehicle`) → **SirenAudio** → Sound | Toggled with **H** and by police AI. |
| `boat_engine_loop.wav` *(loop)* | `BP_Boat` (`GCBoatPawn`) → **EngineAudio** → Sound | Pitch/volume follow throttle/speed. |
| `prop_plane_loop.wav` *(loop)* | `BP_Plane` (`GCPlanePawn`) → **EngineAudio** → Sound | Pitch/volume follow throttle. |
| `jet_airliner_loop.wav` *(loop)* | `BP_Airliner` (`GCAirlinerPawn`) → engine/ambient AudioComponent if added in BP | Cabin/ambient bed; announcements come from the Voice component. |
| `crash_impact.wav` | Optional: play from BP on vehicle hit events | The deformation component handles visuals; use this for impact audio. |
| `door_open.wav` / `door_close.wav` | Optional: BP notify on the enter/exit transition | Short foley thunks for the door pivot animation. |

## Character & world

| File | Where to assign | Notes |
|---|---|---|
| `gunshot.wav` | `BP_Player`/`BP_Ped` → Weapon component → **FireSound** (`GCWeaponComponent::FireSound`) | One-shot per shot. Duplicate the component per weapon if you want variants later. |
| `footstep.wav` | Optional: AnimNotify in locomotion AnimBP | Short tap; randomize pitch in a Sound Cue. |
| `splash.wav` | Optional: swim/water entry events on the character/boat BPs | |
| `waves_loop.wav` *(loop)* | Ambient sound actor at the beach/marina (starter city: west shore) | 6 s swell cycle. |
| `parachute_open.wav` | Optional: hook to `GCParachuteComponent` deploy in BP | Whoosh + canopy flutter. |
| `cash_register.wav` | Optional: shop/purchase/ticket success feedback in BP/UI | |
| `ui_click.wav` / `ui_confirm.wav` | Optional: menu buttons in `BP_MainMenu`/settings children | The C++ widgets are silent by default; add `PlaySound2D` in BP children if desired. |

Nothing in gameplay depends on a sound being assigned — every system runs silently
until you wire these in.
