# Packaging (.exe), updater & website — status and plan

These are later milestone work; gameplay and a verified UE5 compile come first.

## Windows installer

The GitHub release lists `GrandCity-Setup.exe` (about 30 MB). GitHub reports its SHA-256 as
`b99ae9f13e7fb94a950ca0947d707f1ac882d90096a85dc6f47591900d6b1698`, matching the earlier
installer fingerprint documented in the handoff. The installer is a release asset; its NSIS
source (`Tools/Installer/GrandCityInstaller.nsi`) is **not present in the current source tree**,
so its behavior and payload have not been independently verified from source here.

The handoff specifies that this is a **UE5 project installer, not a standalone game**: UE5 and
Visual Studio 2022 are required to build/run the project. Treat the installer as unsigned unless
verified otherwise. Verify the release hash before running it; Windows SmartScreen may warn.

A player-facing packaged-game installer comes only after a verified editor build, cook, and
Shipping package. No standalone game build is currently claimed.

## Updater (planned; not implemented)

- No updater executable or `updater-manifest.example.json` is in the current source tree.
- A future updater must verify every SHA-256 before replacing files, verify a signature where
  available, preserve a rollback copy, and show the changelog. Never run an update that fails
  verification.

## Website (handoff information; not verified in this repository)

The handoff reports a live website at **https://gta-city-alpha.vercel.app**. Website source is not
part of this game repository, and this workspace has not re-verified the current deployment.
The site should remain informational/download-focused: a browser cannot run this UE5 build, and
hosting multiplayer servers requires separate server infrastructure.

The handoff's suggested minimum PC spec is Windows 10/11 64-bit, 6-core CPU, 16 GB RAM, a DX12
GPU with 6 GB+ VRAM, and 20 GB SSD. Revisit this after performance profiling; it is not a
measured guarantee.

## Servers

Current architecture is single-player. Dedicated-server multiplayer is a later roadmap phase;
"servers constantly running" requires separately provisioned hosting.
