# Vehicle tire and engine damage prototype

`AGCVehiclePawn` now tracks per-wheel tire integrity and runtime grip, radiator integrity, engine temperature, overheat misfires, critical-temperature stalls, and repair entry points. These are gameplay/physics prototypes implemented in native C++; they have not been compiled or validated in Unreal Engine 5.8.

## Tire condition

- `TireConditions` is indexed in the same order as the pawn's `Wheels` array.
- High-speed body hits close to a wheel apply tire-integrity damage. `DamageTire(WheelIndex, Amount)` can also be called by weapons, hazards, or Blueprint gameplay (amount is 0–1).
- Grip scales down continuously as integrity falls. At or below 25% integrity a tire is flagged punctured and grip is clamped by `PuncturedTireMinimumGrip`.
- `RepairTires()` restores all wheels. Check `bPunctured`, `Integrity`, and `GripScale` for UI.

## Cooling and failures

- Temperature integrates engine load against speed-assisted radiator cooling.
- Frontal collision damage reduces `RadiatorIntegrity`; cooling capacity scales with that value.
- Above `EngineOverheatTemperatureC`, `EngineMisfireSeverity` rises and drive torque is reduced.
- At `EngineCriticalTemperatureC`, the engine stalls and throttle is cut. It can restart after cooling below the hysteresis threshold (overheat temperature minus 12°C).
- `RepairMechanicalSystems()` restores radiator and tire condition and resets the engine state.
- Optional `EngineSmokeSystem` is spawned at the front of the collision body while overheated or with severe radiator damage. For Niagara intensity response, expose User parameters named `User.HeatIntensity` and `User.RadiatorDamage`; systems that omit these variables simply ignore the updates.

## Tuning and validation

Tune on the vehicle Blueprint: tire friction, impact damage scale, puncture grip floor, engine heat/cooling rates, threshold temperatures, and optional smoke Niagara asset. Validate a clean wheel, damaged wheel, radiator hit, temperature rise under throttle, critical stall, cooldown/restart, and repair cycle in the editor. Unreal Engine/UnrealBuildTool is not available in the present development environment, so no engine build or runtime claim is made.
