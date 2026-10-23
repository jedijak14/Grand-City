# Gridlock loading art

`Resources/LoadingArt/` contains three original generated cinematic loading-screen concepts: a sunset coast, rainy downtown, and harbor at blue hour. The artwork uses broad illustrated open-world key-art language but contains no third-party logos, characters, or copied UI. It is concept art, not a gameplay capture.

`UGCLoadingScreenWidget` cycles through the imported images on each construction and draws its own native loading card and progress bar. The legacy `Docs/LoadingScreenBackdrop.png` remains a fallback/optional override.

1. Build the Unreal project, then run `Tools/ImportModels_Editor.py`. It imports the images into `/Game/Gridlock/UI` alongside the existing art/audio/model groups.
2. To add more screens, place original `.png` art in `Resources/LoadingArt/`, update `BackgroundTextures` in the widget constructor, then re-run the importer.
3. An optional Widget Blueprint child may override `BackgroundTexture` or the default art list.

The imported texture paths and visual result still need UE editor/package validation.
