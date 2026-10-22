# Animation setup — AnimBP wiring for `UGCAnimInstance`

The C++ anim instance already computes every value below each tick from the
owning `AGCCharacter`/vehicle. Your job in the editor is only to build one
Anim Blueprint (parent class **GCAnimInstance**) per imported skeleton and feed
these variables into blend spaces / aim offsets / states. Any humanoid pack
(Fab/Sketchfab/Mixamo-style retarget) works — the gameplay code never changes.

Helper: run `Tools/CreateAnimBlueprint_Editor.py` to create the AnimBP shell
(parent + skeleton) automatically, then add the graph nodes below (5–10 min).

## 1. Locomotion blend space (2D)

Create `BS_Locomotion` on your skeleton:

- Horizontal axis: **Direction** (−180 … 180)
- Vertical axis: **GroundSpeed** (0 … 650)

Samples (use whatever clips your pack calls them):

| Direction \ Speed | 0 | ~170 walk | ~420 jog | ~650 sprint |
|---|---|---|---|---|
| 0 (forward) | Idle | Walk_F | Jog_F | Run_F |
| −90 / +90 | Idle | Walk_L / Walk_R | Jog_L / Jog_R | Jog_L / Jog_R |
| ±180 | Idle | Walk_B | Jog_B | Jog_B |

Alternative: drive a second blend space from **LocalVelocity2D** (X forward,
Y right, cm/s) if your pack is built around velocity-space blending.

Use **BodyLean** (−1…1) on a spine *Transform (Modify) Bone* roll (× ~6°) and
**AccelerationSize** to blend a start/stop lean if your pack has one.
**TurnYawRate** (deg/s) can drive turn-in-place foot planting.

## 2. State machine

States fed by booleans on the anim instance:

- **Locomotion** → `BS_Locomotion`
- **Crouch** when `bCrouching` (single crouch idle/walk pair is enough)
- **Air** when `bFalling` (fall loop; land is handled by returning to locomotion)
- **Swim** when `bSwimming`
- **Parachute** when `bParachuting`
- **Seated** when `bSeated` — driving pose; feed **VehicleSteering** (−1…1)
  into an aim-offset/1D blend of the arms on the wheel, or rotate the
  forearm/hand bones with two-bone IK to the wheel mesh.
- Death is ragdoll from `GCHealthComponent`; `bDead` is exposed if you prefer
  a fall-down clip before ragdoll takes over.

## 3. Aiming (upper body)

- Add an **Aim Offset** fed by **AimYaw** (−90…90, X) and **AimPitch**
  (−80…80, Y) with your rifle/pistol aim poses.
- Blend it in with *Layered Blend Per Bone* (spine_01 up) weighted by
  `bAiming`, so legs keep using the locomotion state machine.

## 4. Enter/exit vehicles

The character code plays a timed door/seat transition (`BeginVehicleTransition`
→ seated). For full body enter/exit montages: create montages and play them
from your `BP_Player`/`BP_Ped` Blueprint on possession change (Event OnPossess /
OnUnPossess or when `bSeated` flips in the AnimBP event graph). Keep root
motion off — the C++ moves the actor to the seat.

## 5. Pedestrians

`BP_Ped` can reuse the same AnimBP: the anim instance only reads velocity and
state from the pawn. Civilians never set `bAiming` unless they are police with
a drawn weapon, which is exactly the behaviour the AI produces.

## 6. Cloth & hair

Assign a Chaos-cloth skeletal mesh on the character's built-in **HoodieMesh**
component and a hair card/groom-style skeletal mesh on **HairMesh** (both are
pose-led by the main mesh in C++). Both are automatically hidden while the
player disables *Cloth and hair physics* in settings
(`UGCGameUserSettings::bClothAndHairPhysics`, applied by
`AGCCharacter::RefreshClothAndHairVisibility`).
