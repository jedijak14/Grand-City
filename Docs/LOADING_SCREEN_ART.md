# Loading-screen and gameplay look concepts

- `LoadingScreenBackdrop.png` is original generated key art for the loading screen: sunset coastal
  city, palms, character, and vehicle. It uses a broad cinematic open-world loading-screen language
  but contains no GTA/Rockstar logos, characters, or copied UI layout.
- `LoadingScreenConcept.png` is a composited UI mockup over that art.
- `GameplayLookConcept.png` is a gameplay-direction mockup with a speculative HUD. It is not a
  capture from a running Unreal build and does not prove that the illustrated city, character,
  car, UI, or effects are implemented.

## Load the backdrop in Unreal

`UGCLoadingScreenWidget` draws its native loading card and progress bar. It first uses a texture
assigned on a Blueprint child; if none is assigned, it tries the imported backdrop asset and falls
back to the built-in skyline if that asset is missing.

1. Build the Unreal project, then run `Tools/ImportModels_Editor.py` once. It imports
   `LoadingScreenBackdrop.png` to `/Game/GrandCity/UI/LoadingScreenBackdrop` alongside the models
   and audio. The widget discovers this fixed asset path automatically.
2. Optionally create a Widget Blueprint derived from `UGCLoadingScreenWidget` and assign another
   `BackgroundTexture` to its class defaults. On the project’s `AGCPlayerController` Blueprint
   child, set `LoadingClass` to that widget to use the override.

The composited PNG is concept art only; the Unreal widget draws its own UI rather than displaying
the concept mockup. The asset import and live loading-screen result still require UE validation.
