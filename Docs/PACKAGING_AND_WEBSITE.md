# Windows workspace setup and packaging status

## Gridlock workspace setup executable

Each published `v*` release is configured to attach three assets from the Windows release workflow:

- `Gridlock-Workspace-Setup.exe` — self-contained Windows x64 setup UI.
- `Gridlock-Workspace.zip` — complete tracked Unreal project/source workspace.
- `SHA256SUMS.txt` — SHA-256 checksums for both files.

The setup application looks for Unreal Engine 5.4 in Epic's registry/launcher metadata and common install roots. It can place the source project into a new user-selected folder (default: `Documents/Unreal Projects/Gridlock`) and launch the detected editor. It extracts the project files, creates `Gridlock.uproject` from the repository descriptor, and leaves the original source module identifiers intact. The installer does not require administrator privileges.

The generated workspace is an **Unreal source project, not a prebuilt game**. Users need Unreal Engine 5.4 and a compatible Visual Studio C++ toolchain to compile and run it. The setup app does not install Unreal Engine, Visual Studio, or third-party dependencies. The project currently has not been verified by an Unreal build or runtime test; no standalone game `.exe` is included or claimed.

The Windows `.exe` is built by `.github/workflows/release-gridlock.yml` when a release is published. This repository environment does not have Windows or the .NET SDK, so the setup application cannot be built or executed locally here. Confirm the release workflow succeeds and its three assets appear before announcing an executable download. The setup executable is not code-signed; Windows SmartScreen may show a warning. Verify hashes against `SHA256SUMS.txt` before running downloaded assets.

## Packaged game (not yet available)

A player-facing standalone game installer requires a successful Unreal compile, cook, and Windows Shipping package. Those steps have not been performed. The setup executable above only deploys the source into an Unreal workspace.

## Updater (not implemented)

There is no updater. Any future updater must verify every SHA-256 before replacing files, verify a signature where available, preserve a rollback copy, and show the changelog. Never run an update that fails verification.

## Website and servers

The prior handoff reports a live website at **https://gta-city-alpha.vercel.app**. Website source is not part of this repository, and this workspace has not re-verified the current deployment. A browser cannot run this Unreal source project; multiplayer hosting also requires separate server infrastructure.
