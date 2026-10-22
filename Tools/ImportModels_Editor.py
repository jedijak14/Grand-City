"""Import repository GLB/FBX models, WAV audio, and original loading-screen backdrop.

Run from Unreal Editor: Tools > Execute Python Script. Requires an enabled GLTF/FBX
importer plugin for the corresponding formats. Source files are never modified.
"""
from pathlib import Path
import unreal

PROJECT_DIR = Path(unreal.Paths.project_dir())
ASSET_GROUPS = (
    (PROJECT_DIR / "ModelLibrary" / "PublicGLB", "/Game/GrandCity/Models", {".glb", ".gltf", ".fbx"}),
    (PROJECT_DIR / "ModelLibrary" / "SketchfabDropIn", "/Game/GrandCity/Imported", {".glb", ".gltf", ".fbx"}),
    (PROJECT_DIR / "AudioSFX", "/Game/Audio/SFX", {".wav"}),
    (PROJECT_DIR / "AudioVoice", "/Game/Audio/Voice", {".wav"}),
)


def import_directory(source_dir, destination, extensions):
    files = sorted(
        str(path) for path in source_dir.rglob("*")
        if path.is_file() and path.suffix.lower() in extensions
    )
    if not files:
        unreal.log_warning("No importable assets found in {}".format(source_dir))
        return []

    tasks = []
    for filename in files:
        task = unreal.AssetImportTask()
        task.filename = filename
        task.destination_path = destination
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported = []
    for task in tasks:
        imported.extend(task.imported_object_paths or [])
        if not task.imported_object_paths:
            unreal.log_warning("Importer produced no asset for {}".format(task.filename))
    return imported


def import_loading_backdrop():
    source = PROJECT_DIR / "Docs" / "LoadingScreenBackdrop.png"
    if not source.is_file():
        unreal.log_warning("Loading backdrop not found: {}".format(source))
        return 0
    unreal.EditorAssetLibrary.make_directory("/Game/GrandCity/UI")
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = "/Game/GrandCity/UI"
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported = task.imported_object_paths or []
    if not imported:
        unreal.log_warning("No Unreal Texture2D was imported for {}".format(source))
    return len(imported)


def main():
    total = 0
    for source, destination, extensions in ASSET_GROUPS:
        total += len(import_directory(source, destination, extensions))
    total += import_loading_backdrop()
    unreal.EditorAssetLibrary.save_directory("/Game/GrandCity", only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_directory("/Game/Audio", only_if_is_dirty=False)
    unreal.log("Grand City import finished: {} asset(s) reported.".format(total))
    unreal.log_warning(
        "If GLB files were skipped, enable the Unreal GLTF importer plugin, restart the editor, "
        "and rerun this script. Confirm each imported asset in Content Browser."
    )


main()
