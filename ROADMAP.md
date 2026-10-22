# Grand City — Roadmap (priority order agreed with the project owner)

## ✅ Phase 1 — Car physics + core sandbox (this delivery, in `Source/`)
- [x] Raycast vehicle physics: suspension, anti-roll, tyre friction circle, drift/handbrake,
      gearbox/FWD/RWD/AWD, speed-sensitive Ackermann steering, engine/screech audio hooks
- [x] Skidmarks, crash mesh deformation + damage tiers, chase + cockpit (first-person) cameras
- [x] Character: walk/sprint/jump/swim, aim/shoot (4 weapon types), ragdoll death → hospital respawn
- [x] Animated door + seat transition for enter/exit (car; instant for plane/boat in phase 1)
- [x] NPC pedestrians (wander/flee/ragdoll), traffic AI drivers, police (foot + car pursuit,
      arrest at 1–2★, shooting at 3★+, wanted decay when hidden)
- [x] Day/night cycle + real-constellation star dome + moon
- [x] Ocean wave field shared by boats + swimming
- [x] Locations: hospital, robbable shop, police station, airport (booked flight), marina, roof elevator
- [x] Parachute simulation (deploy/glide/flare/canopy sway)
- [x] Flyable plane (thrust/lift/stall/flaps/rudder/gear), boat (buoyancy)
- [x] HUD (money, wanted, speed/gear/RPM, health/ammo, minimap, WASTED), dev console commands
- [x] Settings data (blood/waves/grass/cloth toggles, densities, day length) wired into gameplay
- [x] 15 public GLB starter models + Sketchfab drop-in import pipeline + starter city script

## ✅ Phase 1.5 — Water, air & airports (second delivery)
- [x] Ocean sample upgrade: wave velocity, currents, foam factor; boats ride moving swells,
      planing lift + bow spray hooks, anchor, smoothed throttle, engine audio
- [x] Flyable plane upgrade: fuel + refuel, ground effect, spoilers, wheel brakes, stall state, engine audio
- [x] Airport computers: flight offers, seat classes/pricing, booking, check-in (seat/gate),
      boarding pass in player wallet, staff voice greetings
- [x] Passenger airliner: gated boarding with ticket validation, NPC passengers, flight phases
      with crew/captain announcements, seatback entertainment (map/movies/music/games)
- [x] Voice-line system + generated VO batch 1 (check-in, cabin crew, captain, NPC ambient)
- [x] Real offline renders of all starter models + visual target screenshots (Docs/Screenshots)

## ✅ Phase 2 — Feel & visuals (code complete; editor asset wiring remains)
- [x] Surfboard / paddleboard / white-water raft pawn (`GCSurfboardPawn`) on the shared wave API,
      with balance/wipeouts, beach detection and HUD readout; starter-city spawns at the west beach
- [x] Grass foliage actor (`GCGrassFieldActor`, HISM + per-instance wind phase, settings-gated)
- [x] Marine life (`GCMarineLifeActor`: instanced fish school + leaping dolphins on the wave field)
- [x] UMG main menu + settings screen + GTA-style loading screen (asset-free C++ widgets),
      wired through `GCPlayerController` (`GCMenu` / `GCSettingsMenu` / `GCLoadingTest`, optional menu-on-start)
- [x] Crouch (hold C), expanded `GCAnimInstance` data (local velocity, lean, aim offsets,
      crouch/dead, vehicle steering) + `Docs/ANIMATION_SETUP.md` + AnimBP shell script
- [x] Synthesized SFX set (17 WAVs in `AudioSFX/`, generator `Tools/GenerateSFX.py`, wiring `Docs/SFX.md`)
- [x] Voice batch 2 recorded — all 20 scripted lines now have audio (`AudioVoice/`)
- [x] Cloth/hair settings toggle now consumed by the character (HoodieMesh/HairMesh visibility)
- [ ] AnimBP graph assets per imported skeleton (editor work — variables/docs/script provided)
- [ ] Niagara FX assets: tyre smoke, muzzle flash, blood, crash sparks, boat wake (C++ hooks exist)
- [ ] Passenger-flight seatback as a UMG widget (currently canvas HUD); second island / OSM city pass

## Phase 3 — Life-sim & shipping (deprioritized by owner, still planned)
- Job loops: cop (arrest missions), hitman (targets), taxi, medic, shop owner, border patrol,
  president (dev command exists already), hiring flow + payouts; phone UMG app suite
- Packaging: UE5-project installer .exe (NSIS, installs into your UE5 projects folder) → later a
  packaged-game Shipping build + hash-verified updater. Vercel site is LIVE (see Docs/PACKAGING_AND_WEBSITE.md)

## Phase 4 — Online
- Replication pass on pawns/components, dedicated server target, session hosting

## Honest limits (unchanged)
- True BeamNG-class soft-body is approximated with permanent mesh denting (see GCDeformationComponent).
- "UE5-level graphics" come from the assets/lighting YOU import — the code enables them, grey-box is only a test bed.
- AAA scale/polish (Rockstar budget/team) is not achievable in this format; this roadmap targets a strong playable sandbox.
