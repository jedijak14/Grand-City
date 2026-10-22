# Project status / handoff (read this first when resuming)

**State:** Milestone 1 + 1.5 + Phase 2 C++ source in `Source/` (menus/settings/loading widgets,
surf/paddle/raft, grass, marine life, crouch, expanded anim data) + 20 voice lines + 17 SFX.
Website is LIVE on Vercel (gta-city-alpha.vercel.app). A Windows NSIS installer (.exe) now
installs the project into an existing UE5 setup (see Tools/Installer/).
Priority (owner-confirmed): **car physics + core sandbox first**, then boats/water,
planes, airport computers/tickets, voice lines; jobs/life-sim deprioritized.
Full design/roadmap: `README.md`, `ROADMAP.md`. Model plan: `Docs/MODEL_SOURCES.md`.
Screenshots: `Docs/Screenshots/` (REALMODEL_* = true renders of included GLBs,
TARGET_* = AI concept targets, not gameplay). Voice: `Docs/VOICE_LINES.md` + `AudioVoice/`.

**Blocker / next action:** Compile in UE 5.4 + VS2022 on the owner's Windows PC
(the authoring sandbox has no Unreal). Then:
1. Paste the first compiler error list back to the assistant → fix rounds.
2. Create empty level → run `Tools/BuildStarterCity_Editor.py` → Play (F = enter car).
3. Run `Tools/ImportModels_Editor.py`, make BP children, assign meshes (README §3).

**Storage/GitHub:** Local git repo initialized; code+docs committed. The 15 GLB models
(33 MB, `ModelLibrary/PublicGLB/`) are NOT in git yet — they need Git LFS
(`git lfs install`, then `git add ModelLibrary`). GitHub push requires the owner to
connect a GitHub account/remote; sandbox has no GitHub credentials. Workspace also
contains an abandoned web prototype folder (`../open-world-game`, ~1.5 MB) — safe to delete.

**Known simplifications (by design, see ROADMAP):** crash soft-body = mesh denting;
AnimBP graph assets and Niagara FX assets are editor wiring (C++ data/hooks + docs provided);
jobs/phone are phase 3; single-player (multiplayer phase 4). The .exe installs the UE5
*project* (UE5 + VS2022 still required to compile) — a standalone packaged game build
comes after the first verified compile/package on the owner's PC.
