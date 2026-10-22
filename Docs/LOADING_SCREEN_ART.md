# Loading-screen and gameplay look concepts

- `LoadingScreenBackdrop.png` is original generated key art for the loading screen. It has no
  game logos or third-party character designs.
- `LoadingScreenConcept.png` is a composited UI mockup over that art.
- `GameplayLookConcept.png` is a gameplay-direction mockup with a speculative HUD. It is not a
  capture from a running Unreal build and does not prove that the illustrated city, character,
  car, UI, or effects are implemented.

## Use the loading art in Unreal

The native `UGCLoadingScreenWidget` exposes an optional `BackgroundTexture`. The C++ widget draws
its built-in skyline when this field is empty; a supplied texture is stretched to the screen
behind the loading card.

1. Build the project in Unreal Editor, then import `LoadingScreenBackdrop.png` into
   `/Game/GrandCity/UI/` as a `Texture2D` (UI texture group; do not compress as normal maps).
2. Create a Widget Blueprint derived from `UGCLoadingScreenWidget` and assign the imported
   texture to its `BackgroundTexture` class-default property.
3. On the project’s `AGCPlayerController` Blueprint child, set `LoadingClass` to that widget
   Blueprint. `ShowLoadingScreen` then displays the art with the native progress panel.

This import/Blueprint setup has not been executed in this workspace. Keep the composited PNGs as
concept images; the Unreal widget renders its own UI rather than displaying the composited PNG.
