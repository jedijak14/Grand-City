# Packaging (.exe), updater & website — status and plan

These are **milestone 3** by design (gameplay first). Here's the concrete path so
nothing is hand-waved later:

## Windows .exe

**Current (Phase 2): UE5-project installer** — `GrandCity-Setup.exe` (NSIS,
script `Tools/Installer/GrandCityInstaller.nsi`, ~30 MB). Per the owner's
spec it *connects through Unreal Engine*: **UE5 must already be installed**
(+ Visual Studio 2022). It detects the engine (registry/default Epic folder,
prefers 5.4), installs the project into `Documents\Unreal Projects\GrandCity`,
optionally runs `UnrealVersionSelector /projectfiles`, creates editor
shortcuts + an uninstaller, and drops `INSTALL_README.txt` with build steps.
First Phase-2 build SHA-256:
`b99ae9f13e7fb94a950ca0947d707f1ac882d90096a85dc6f47591900d6b1698`.
It is **unsigned** (no code-signing cert), so Windows SmartScreen will warn —
*More info → Run anyway*, and verify the hash above.

**Later: packaged game .exe** (after first verified compile on the owner's PC):
1. In the editor: Platforms > Windows > Package Project → **Shipping** build.
2. Wrap with the same NSIS approach for a player-facing installer.
3. First-run prerequisites (UE prerequisites installer) are bundled by the packager.

## Updater (planned design, safe by default)
- `updater-manifest.example.json` (in Tools/) defines: version, file list, SHA-256 per file.
- The updater (small C++/PowerShell companion) must: verify every hash **before**
  replacing files, verify an Authenticode/package signature where available,
  keep the previous build for rollback, and show the changelog from the manifest.
- Never run an update file that fails hash verification.

## Website (Vercel) — LIVE (22 Oct 2026)
- Production: **https://gta-city-alpha.vercel.app** (Vercel project `gta-city`,
  deployment `dpl_BqNGbZWJvHP9q3mXUsEyGDqRF5Xw`, READY/PROMOTED, Next.js on Node 24.x).
- Source lives in the workspace at `GrandCityWebsite/`; push it to `jedijak14/gta-city`
  `main` to make Git the source of truth (see that folder's README — the first deployment
  was made directly through the Vercel connector, and Git-push deployments may need
  enabling under Vercel → Project → Settings → Git).
- Site covers features/systems, labelled media, controls, hardware requirements,
  roadmap, source-build steps and FAQ. The Windows .exe button stays disabled until
  the first verified packaged build exists.
- A website **cannot host the game server** and a browser **cannot run this UE5 build** —
  the site is for downloads/info only. (Pixel Streaming is an option later, needs a GPU server.)
- Suggested minimum spec to publish (revisit after profiling): Windows 10/11 64-bit,
  6-core CPU, 16 GB RAM, GTX 1660 / RX 5600-class GPU, 20 GB SSD.

## Servers
- Current build: single-player. UE dedicated-server multiplayer is roadmap phase 4;
  "servers constantly running" requires paid hosting you control (e.g. a VPS) —
  the game code will support a dedicated server target when we get there.
