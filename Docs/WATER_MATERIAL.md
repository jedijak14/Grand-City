# Water: making visuals match the physics

`AGCOceanManager` is the single source of truth for the three travelling sine waves
(amplitudes 26/15/7 cm): height, analytic surface normal, vertical velocity, horizontal
wave-orbital velocity, background current, and a crest/foam estimate. Boats, swimmers,
and surfboards cache the nearest ocean manager so its actor Z and amplitude setting are
respected. Boat buoyancy now applies capped multi-point spring lift and damping against
local wave/current velocity; board steering responds to combined surface flow. These
are still approximations, not a full fluid or CFD solver.

To make the visible surface match, either:

**Option A (quick):** Water material with World Position Offset in the material graph —
rebuild the same 3 sine waves with `Time`, `WorldPosition` nodes:
```
H = 26*sin(0.0016*X + 0.0011*Y + 0.9*T)
  + 15*sin(-0.0023*X + 0.0028*Y + 1.35*T)
  +  7*sin(0.0061*X - 0.0052*Y + 2.2*T)
WPO.Z = H   (mesh needs enough subdivisions - use a 128x128+ plane near shore)
```

**Option B (later, bigger):** UE Water plugin (WaterBodyOcean with Gerstner waves).
Then replace `GetWaveHeightAt` internals with the water body's height query so
surfing/paddleboarding/rafting (roadmap) read the same surface.

Surf/paddleboard plan (roadmap phase 2): a `GCSurfboardPawn` sampling
`GetWaveHeightAt`/`GetWaveNormalAt` at 4 points, slope-powered acceleration,
balance from steer input. The wave API above is already shaped for it.
