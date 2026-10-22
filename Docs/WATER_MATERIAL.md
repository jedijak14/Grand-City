# Water: shared surface queries, deformation, and wakes

`AGCOceanManager` owns the analytic base wave field (26/15/7 cm travelling waves), local
surface normals and velocities, current, and crest estimate. Boat buoyancy, swimmer/surfboard
queries, and the optional harbor mesh use that same field. Boat lift is still a capped
multi-point spring/damping approximation; it is not a full fluid, CFD, or pressure solver.

## Harbor visualization

The optional `WaveSurface` is a fixed-resolution `UProceduralMeshComponent`. It deforms the
surface to the base wave plus active wake field and derives normals from the displaced grid.
The Harbor Vertical Slice enables a 192-cell grid across a 600 m by 1 km footprint and a roughly
30 Hz update interval. Its
water material uses the vertex red channel as a crest/wake foam mask, blending toward lighter,
rougher water on steep crests. This CPU update path is a visual-development implementation;
profile it in UE before using it across a large world or on lower-end hardware. A GPU WPO or
UE Water implementation may be preferable at production scale, but must keep query and visual
waves synchronized.

## Wake disturbances

`EmitWake` adds bounded, short-lived wake packets to the manager. Each packet produces a pair
of diverging wave arms behind its source with spreading length, wavelength, amplitude, and decay.
The same wake height is sampled by gameplay and rendered by `WaveSurface`; sampled wake slope
and vertical speed contribute to buoyancy, while the vertex foam mask brightens the crest.
Boats emit packets from the stern at speed-dependent strength (more when planing). The flyable
plane emits weaker packets only during a fast, low pass over the configured ocean surface.

These are stylized surface disturbances, not hydrodynamically exact boat wakes or aircraft
pressure fields. The mesh must cover the effect and be enabled in the level for disturbances to
be visible. Physics, performance, visual quality, and UE 5.4 API compatibility remain unverified
until compiled and exercised in Unreal.
