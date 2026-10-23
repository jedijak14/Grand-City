# Police Career Implementation Notes

## Implemented in source (not Unreal-verified)

- E is tap-to-interact. For an on-duty police officer targeting a pedestrian or vehicle, holding E for 0.28 seconds opens the contextual wheel; release closes it. A pedestrian is stopped when the wheel opens. The controller also refuses to show the wheel unless the player is on an active police shift.
- The pedestrian and traffic wheels include ID/record checks, interviews, citations, searches/arrests/releases, alcohol/drug tests, and traffic-stop actions. Action markers use text tokens rather than emoji.
- Pedestrians receive generated identity/license/record data. Some generated IDs fail lookup and some licenses are expired. Citations check those facts before they can be recorded.
- Pedestrian impairment is represented with observable slurred-speech/unsteady-gait flags; voice playback pitch is lowered for slurred speakers when an audio clip is assigned, and movement speed is reduced for unsteady pedestrians. Alcohol/drug tests require the matching police kit, a stop, and observable cues.
- Ambient AI traffic has a low chance of impaired drivers and equipment defects. A traffic stop records the captured speed and observed impairment cues. Vehicle accident collisions record witness/victim statements on nearby pedestrians for later interviews.
- Police shifts grant test kits and a persistent badge number. The HUD displays the badge number and citizen name. Firing a firearm during an active police shift incurs conduct loss and wanted stars; police pursuit AI attempts arrest. A successful arrest transfers the player to a police station and applies a short, timed custody penalty. The taser is bound to 5 and knocks pedestrians down with low damage.
- Shop and police-station location actors can randomly offer a job. Interact with E, then press J to accept (“I'm here about the job”) or N to decline. Job changes are saved.

## Unreal asset setup still required

1. In the player character Blueprint's `OutfitPresets`, add a preset such as `PolicePatrol_Navy`, with `Category = Uniform`, a compatible body mesh and a clothing skeletal mesh using the same skeleton, and a display name. The code cannot supply a finished uniform mesh/material; the navy short-sleeve patrol uniform reference is an asset-authoring target. Use fictional Gridlock insignia rather than real department logos.
2. The police badge number/name currently appear in the HUD, not on a modeled badge. Add a badge/accessory mesh and a material or text-render component in Unreal if a physically engraved badge is needed.
3. Assign appropriate alcohol/drug test tools and visual/audio assets in Unreal. The code tracks possession of test kits but does not yet provide a modeled handheld test device.
4. Populate each pedestrian Blueprint's `GCVoiceLineComponent.Lines` with imported `Sound` assets and subtitles. The code supports voice playback and a slurred playback pitch, but does not include 500 recorded voice clips.

## Remaining police scope

A dedicated tablet/phone database screen, real dispatch/ambulance/tow workflows, pursuit PIT/spike-strip systems, persistent case/evidence management, a full incident director (including school events), and authored uniform/badge/tool assets remain unfinished. Traffic offense conditions still depend on vehicle data/AI behavior rather than a complete traffic-law simulation. Unreal compilation, map integration, and runtime testing are still required.
