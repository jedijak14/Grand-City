# Quaternius character import, retargeting, and cloth setup (UE 5.8)

## Verified source data

`Tools/ValidateQuaterniusWardrobe.py` confirms the six bundled models each contain 24 animation clips and 62 joints. Their joint names are shared across the selected models (`Root`, `Body`, `Hips`, `Abdomen`, `Torso`, `Chest`, `Shoulder.L/R`, `UpperArm.L/R`, `LowerArm.L/R`, `Wrist.L/R`, `UpperLeg.L/R`, `LowerLeg.L/R`, `Foot.L/R`, `PT.L/R`, plus four-bone finger chains). The source package uses separate skinned mesh parts per character. This structural test does not establish that UE imports them as a single usable skeletal mesh.

## Import and retarget checklist

1. Build the `GrandCityEditor` target in Unreal Engine 5.8, enable the GLTF importer, restart the editor, and run `Tools/ImportModels_Editor.py`.
2. Inspect every imported GLB. Determine whether the importer made one shared Skeleton plus multiple Skeletal Mesh assets or duplicated Skeleton assets. Reassign parts to one canonical Quaternius Skeleton if needed; keep the head/body/clothing parts together as a character set.
3. Create an IK Rig for the Quaternius Skeleton and another for the project character's actual target Skeleton. The target cannot be assumed: no player Blueprint/Skeleton asset is committed in this checkout.
4. Add matching IK Retargeter chains. Minimum source-to-target intent for UE Manny/Quinn naming is:
   - `Root` -> `root`
   - `Hips` -> `pelvis`
   - `Abdomen`/`Torso`/`Chest` -> `spine_01`/`spine_02`/`spine_03`
   - `Neck`/`Head` -> `neck_01`/`head`
   - `Shoulder.L`, `UpperArm.L`, `LowerArm.L`, `Wrist.L` -> `clavicle_l`, `upperarm_l`, `lowerarm_l`, `hand_l` (same for `.R` -> `_r`)
   - `UpperLeg.L`, `LowerLeg.L`, `Foot.L`, `PT.L` -> `thigh_l`, `calf_l`, `foot_l`, `ball_l` (same for `.R` -> `_r`)
   - Map finger chains per digit only if the target Skeleton has matching bones. Preserve the Quaternius embedded animations for source-skeleton use until the target retargeter passes pose, root-motion, foot-sliding, and hand-contact checks.
5. Build an AnimBP per target skeleton or use a verified retargeted locomotion set. Do not assign an AnimBP whose Skeleton differs from the active mesh.
6. Populate `AGCCharacter::OutfitPresets` in the actual player Blueprint only after mesh/skeleton assembly is proven. Keep swimwear selections constrained by `bAppropriateCoverage` and profile sex compatibility. A complete-character GLB is not a standalone clothing overlay.

## Chaos Cloth authoring checklist

Chaos cloth simulation is authored data on a Skeletal Mesh section, not a switch that can turn any arbitrary GLB triangles into clothing at runtime. For each garment that is actually intended to simulate:

1. Use a garment mesh skinned to the final character Skeleton with clean topology, UVs, and appropriate material sections. Keep rigid footwear, belts, badges, and equipment out of the simulated section.
2. In UE 5.8, create Clothing Data from the garment section, configure the cloth asset, and paint/verify a max-distance map: zero or near-zero at shoulders/waist attachment edges, increasing toward loose hems and panels. Avoid simulating the whole body.
3. Tune cloth mass, damping, tether stiffness, edge constraints, gravity, wind response, collision thickness, and self-collision per garment. Add/validate Physics Asset bodies and collision masks around pelvis, torso, thighs, and arms.
4. Test walk/run/sprint, crouch, jump, swim, vehicle transitions, ragdoll/death, pause, LOD changes, and 30/60/120 FPS. Check for clipping, explosive vertices, cloth tunneling, and CPU/GPU cost on target hardware.
5. Only then connect the garment to an `FGCOutfitPreset` and expose the project's cloth quality setting. Never claim Chaos Cloth works based only on a visible static mesh or this checklist.

## Runtime C++ control now present

`UGCClothPhysicsLibrary::ApplyClothSimulation` applies the global setting to every skeletal mesh component on an actor: it toggles `bDisableClothSimulation`, cloth blend weight, max-distance scale, and suspend/resume calls. The character and pedestrian begin-play paths call it; changing the settings checkbox applies the change immediately to existing player and pedestrian actors. Disabling simulation leaves garments visible in their skinned pose instead of hiding them. `AGCCharacter::ClothMaxDistanceScale` exposes per-character max-distance scaling.

This is runtime control of Unreal's built-in Chaos Cloth simulation, not an asset generator. It will simulate only meshes whose sections already have valid Clothing Data and a compatible physics setup.

## Current status / blocker

The project has no committed Unreal Content directory, player Blueprint, target Skeleton, IK Rig, IK Retargeter, Physics Asset, or authored Chaos Clothing Data assets, and this workspace has no Unreal Editor/BuildTool. The source-side simulation controls are implemented but uncompiled/unrun; clothing still needs authored data, skeleton retargeting, and verification in UE 5.8 before it can be marked complete.
