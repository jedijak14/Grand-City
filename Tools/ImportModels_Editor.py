"""Import project source art, models and original audio into the Unreal content browser."""
import os
from pathlib import Path
import unreal

PROJECT_DIR = Path(unreal.Paths.project_dir())
ASSET_GROUPS = (
    (PROJECT_DIR / "ModelLibrary" / "PublicGLB", "/Game/GrandCity/Models", {".glb", ".gltf", ".fbx"}),
    (PROJECT_DIR / "ModelLibrary" / "SketchfabDropIn", "/Game/GrandCity/Imported", {".glb", ".gltf", ".fbx"}),
    (PROJECT_DIR / "ModelLibrary" / "Clothing", "/Game/Gridlock/Characters/Clothing", {".glb", ".gltf", ".fbx"}),
    (PROJECT_DIR / "AudioSFX", "/Game/Audio/SFX", {".wav"}),
    (PROJECT_DIR / "AudioVoice", "/Game/Audio/Voice", {".wav"}),
)


def import_directory(source, destination, extensions):
    if not source.is_dir():
        unreal.log_warning("Source folder not found: {}".format(source))
        return []
    unreal.EditorAssetLibrary.make_directory(destination)
    tasks = []
    for path in sorted(source.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in extensions:
            continue
        task = unreal.AssetImportTask()
        task.filename = str(path)
        task.destination_path = destination
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    if not tasks:
        return []
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported = []
    for task in tasks:
        imported.extend(task.imported_object_paths or [])
        if not task.imported_object_paths:
            unreal.log_warning("Importer produced no asset for {}".format(task.filename))
    return imported


def import_loading_art():
    candidates = [PROJECT_DIR / "Docs" / "LoadingScreenBackdrop.png"]
    loading_dir = PROJECT_DIR / "Resources" / "LoadingArt"
    if loading_dir.is_dir():
        candidates.extend(sorted(loading_dir.glob("*.png")))
    unreal.EditorAssetLibrary.make_directory("/Game/Gridlock/UI")
    tasks = []
    for source in candidates:
        if not source.is_file():
            continue
        task = unreal.AssetImportTask()
        task.filename = str(source)
        task.destination_path = "/Game/Gridlock/UI"
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    if not tasks:
        unreal.log_warning("No Gridlock loading art was found to import.")
        return 0
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported = 0
    for task in tasks:
        imported += len(task.imported_object_paths or [])
        if not task.imported_object_paths:
            unreal.log_warning("No loading texture imported for {}".format(task.filename))
    return imported


def main():
    total = 0
    for source, destination, extensions in ASSET_GROUPS:
        total += len(import_directory(source, destination, extensions))
    total += import_loading_art()
    unreal.EditorAssetLibrary.save_directory("/Game/GrandCity", only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_directory("/Game/Gridlock", only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_directory("/Game/Audio", only_if_is_dirty=False)
    unreal.log("Gridlock import finished: {} asset(s) reported.".format(total))
    unreal.log_warning(
        "If GLB files were skipped, enable the Unreal GLTF importer plugin, restart the editor, "
        "and rerun this script. Confirm each imported asset in Content Browser."
    )


main()
