"""Create empty AnimBlueprint shells for imported skeletons.

Run in Unreal Editor after ImportModels_Editor.py. The state-machine/blend-space graph
is intentionally not fabricated; see Docs/ANIMATION_SETUP.md for editor wiring steps.
"""
import unreal

SOURCE_PATH = "/Game/GrandCity/Models"
OUTPUT_PATH = "/Game/GrandCity/Animation"
PARENT_CLASS = unreal.load_class(None, "/Script/GrandCity.GCAnimInstance")


def main():
    if not PARENT_CLASS:
        raise RuntimeError("GCAnimInstance class not loaded; compile the GrandCity Editor target first.")

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path(SOURCE_PATH, recursive=True)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    created = 0

    for asset_data in assets:
        asset = asset_data.get_asset()
        if not isinstance(asset, unreal.Skeleton):
            continue
        skeleton_name = asset.get_name()
        asset_name = "ABP_" + skeleton_name
        object_path = OUTPUT_PATH + "/" + asset_name + "." + asset_name
        if unreal.EditorAssetLibrary.does_asset_exist(object_path):
            unreal.log("AnimBlueprint already exists: {}".format(object_path))
            continue

        factory = unreal.AnimBlueprintFactory()
        factory.set_editor_property("target_skeleton", asset)
        factory.set_editor_property("parent_class", PARENT_CLASS)
        blueprint = tools.create_asset(asset_name, OUTPUT_PATH, unreal.AnimBlueprint, factory)
        if blueprint:
            created += 1
            unreal.log("Created AnimBlueprint shell {}".format(object_path))
        else:
            unreal.log_warning("Could not create AnimBlueprint for {}".format(skeleton_name))

    unreal.EditorAssetLibrary.save_directory(OUTPUT_PATH, only_if_is_dirty=False)
    unreal.log("Created {} AnimBlueprint shell(s). Wire graphs per Docs/ANIMATION_SETUP.md.".format(created))


main()
