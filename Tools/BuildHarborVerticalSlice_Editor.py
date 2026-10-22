"""Build an art-directed coastal vertical-slice map using Unreal Editor's built-in meshes.

Run after compiling GrandCity in UE 5.4: Tools > Execute Python Script. This creates
/Game/Maps/L_HarborVerticalSlice plus reusable flat-color PBR materials in /Game/GrandCity.
It is intentionally a playable look-development blockout, not a claim of final AA art.
"""
from pathlib import Path
import math
import unreal

MAP_PATH = "/Game/Maps/L_HarborVerticalSlice"
MATERIAL_DIR = "/Game/GrandCity/Materials/Harbor"
PROJECT_DIR = Path(unreal.Paths.project_dir())

PALETTE = {
    "asphalt": ((0.045, 0.060, 0.070), 0.88, 0.0),
    "sidewalk": ((0.34, 0.35, 0.32), 0.82, 0.0),
    "limestone": ((0.70, 0.55, 0.38), 0.76, 0.0),
    "stucco": ((0.78, 0.70, 0.57), 0.78, 0.0),
    "coral": ((0.52, 0.20, 0.14), 0.78, 0.0),
    "teal": ((0.08, 0.28, 0.28), 0.72, 0.0),
    "glass": ((0.035, 0.14, 0.19), 0.20, 0.28),
    "trim": ((0.87, 0.75, 0.55), 0.42, 0.0),
    "paint": ((0.85, 0.77, 0.60), 0.64, 0.0),
    "palm": ((0.19, 0.25, 0.16), 0.88, 0.0),
    "leaf": ((0.08, 0.22, 0.13), 0.76, 0.0),
    "metal": ((0.12, 0.16, 0.17), 0.28, 0.55),
    "water": ((0.018, 0.105, 0.135), 0.16, 0.18),
    "water_dynamic": ((0.018, 0.105, 0.135), 0.16, 0.18),
    "amber": ((0.98, 0.46, 0.12), 0.3, 0.0),
    "white": ((0.91, 0.88, 0.77), 0.58, 0.0),
}


def native_class(name):
    cls = unreal.load_class(None, "/Script/GrandCity." + name)
    if not cls:
        raise RuntimeError("Build the GrandCity Editor target before running this script (missing {}).".format(name))
    return cls


def safe_set(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception:
        return False


def create_material(name, color, roughness, metallic):
    object_path = MATERIAL_DIR + "/M_" + name
    if unreal.EditorAssetLibrary.does_asset_exist(object_path):
        return unreal.EditorAssetLibrary.load_asset(object_path)
    factory = unreal.MaterialFactoryNew()
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_" + name, MATERIAL_DIR, unreal.Material, factory
    )
    if not material:
        raise RuntimeError("Could not create material {}".format(object_path))

    graph = unreal.MaterialEditingLibrary
    base = graph.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -420, -80
    )
    base.set_editor_property("constant", unreal.LinearColor(color[0], color[1], color[2], 1.0))
    rough = graph.create_material_expression(
        material, unreal.MaterialExpressionConstant, -420, 100
    )
    rough.set_editor_property("r", roughness)
    metal = graph.create_material_expression(
        material, unreal.MaterialExpressionConstant, -420, 230
    )
    metal.set_editor_property("r", metallic)
    if name == "water_dynamic":
        # Vertex red carries crest/wake foam: blend toward white and increase roughness at disturbed crests.
        vertex_color = graph.create_material_expression(
            material, unreal.MaterialExpressionVertexColor, -180, 360
        )
        foam_white = graph.create_material_expression(
            material, unreal.MaterialExpressionConstant3Vector, -180, 20
        )
        foam_white.set_editor_property("constant", unreal.LinearColor(0.82, 0.93, 0.94, 1.0))
        color_blend = graph.create_material_expression(
            material, unreal.MaterialExpressionLinearInterpolate, 100, -60
        )
        graph.connect_material_expressions(base, "", color_blend, "A")
        graph.connect_material_expressions(foam_white, "", color_blend, "B")
        graph.connect_material_expressions(vertex_color, "R", color_blend, "Alpha")
        rough_foam = graph.create_material_expression(
            material, unreal.MaterialExpressionConstant, -180, 190
        )
        rough_foam.set_editor_property("r", 0.78)
        rough_blend = graph.create_material_expression(
            material, unreal.MaterialExpressionLinearInterpolate, 100, 160
        )
        graph.connect_material_expressions(rough, "", rough_blend, "A")
        graph.connect_material_expressions(rough_foam, "", rough_blend, "B")
        graph.connect_material_expressions(vertex_color, "R", rough_blend, "Alpha")
        graph.connect_material_property(color_blend, "", unreal.MaterialProperty.MP_BASE_COLOR)
        graph.connect_material_property(rough_blend, "", unreal.MaterialProperty.MP_ROUGHNESS)
    else:
        graph.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
        graph.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    graph.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    graph.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def build_materials():
    unreal.EditorAssetLibrary.make_directory(MATERIAL_DIR)
    return {
        name: create_material(name, *spec)
        for name, spec in PALETTE.items()
    }


def spawn_mesh(mesh, location, scale, material, label, rotation=None, collision=True):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(*location),
        rotation or unreal.Rotator(0.0, 0.0, 0.0),
    )
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(mesh)
    component.set_world_scale3d(unreal.Vector(*scale))
    if material:
        component.set_material(0, material)
    component.set_mobility(unreal.ComponentMobility.STATIC)
    if collision:
        component.set_collision_profile_name("BlockAll")
    else:
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    actor.set_actor_label(label, mark_dirty=True)
    return actor


def box(cube_mesh, mats, key, location, scale, label, rotation=None, collision=True):
    return spawn_mesh(cube_mesh, location, scale, mats[key], label, rotation, collision)


def make_facade(cube_mesh, mats, sign, y, floors, bays, width, depth, height, palette_key, name):
    x = sign * 5300.0
    box(cube_mesh, mats, palette_key, (x, y, height * 0.5),
        (depth / 100.0, width / 100.0, height / 100.0), "HARBOR - {} facade mass".format(name))
    inner_face = x - sign * (depth * 0.5 + 6.0)
    # Deep reveals and warm metal frames create visible window rhythm without texture assets.
    bay_step = width / float(bays + 1)
    floor_step = (height - 650.0) / max(1, floors)
    for floor in range(floors):
        z = 430.0 + floor * floor_step
        for bay in range(1, bays, 3):
            wy = y - width * 0.5 + bay * bay_step
            box(cube_mesh, mats, "glass", (inner_face, wy, z),
                (0.16, min(2.7, bay_step * 0.0068), min(2.5, floor_step * 0.0062)),
                "HARBOR - {} recessed glazing".format(name), collision=False)
    # Strong retail podium, cornice and rooftop setback make silhouettes legible from the street.
    box(cube_mesh, mats, "trim", (x - sign * (depth * 0.5 + 12.0), y, 250.0),
        ((depth + 24.0) / 100.0, (width + 80.0) / 100.0, 0.80), "HARBOR - {} podium belt".format(name))
    box(cube_mesh, mats, "trim", (x, y, height + 70.0),
        ((depth + 130.0) / 100.0, (width + 130.0) / 100.0, 0.95), "HARBOR - {} roof cornice".format(name))
    for side_y in (-1.0, 1.0):
        box(cube_mesh, mats, "trim", (inner_face - sign * 18.0, y + side_y * width * 0.47, height * 0.51),
            (0.24, 0.70, height * 0.0094), "HARBOR - {} facade pilaster".format(name))
    # A shaded storefront and canvas awning at street level.
    box(cube_mesh, mats, "glass", (inner_face - sign * 28.0, y, 170.0),
        (0.25, width * 0.0066, 2.55), "HARBOR - {} storefront glazing".format(name), collision=False)
    box(cube_mesh, mats, "coral", (inner_face - sign * 110.0, y, 355.0),
        (0.45, min(12.0, width * 0.004), 0.90), "HARBOR - {} blade sign".format(name), collision=False)
    box(cube_mesh, mats, "amber", (inner_face - sign * 170.0, y, 525.0),
        (1.80, width * 0.0045, 0.25), "HARBOR - {} awning".format(name), collision=False)


def add_palm(cube_mesh, sphere_mesh, mats, x, y, index):
    box(cube_mesh, mats, "palm", (x, y, 405.0), (0.46, 0.46, 8.0),
        "HARBOR - palm trunk {:02d}".format(index))
    # Splayed broad fronds, deliberately simple but readable in the street silhouette.
    for frond in range(7):
        yaw = frond * (360.0 / 7.0)
        angle = 38.0 + (frond % 3) * 8.0
        direction = unreal.Rotator(-angle, yaw, 0.0)
        spawn_mesh(sphere_mesh, (x, y, 825.0), (0.85, 4.8, 0.38), mats["leaf"],
                   "HARBOR - palm canopy {:02d} frond {:02d}".format(index, frond), direction, collision=False)


def add_street_light(cube_mesh, sphere_mesh, mats, x, y, index):
    box(cube_mesh, mats, "metal", (x, y, 480.0), (0.24, 0.24, 9.5),
        "HARBOR - mast {:02d}".format(index))
    box(cube_mesh, mats, "metal", (x + 125.0, y, 925.0), (2.60, 0.20, 0.18),
        "HARBOR - outreach arm {:02d}".format(index))
    box(cube_mesh, mats, "amber", (x + 245.0, y, 900.0), (1.0, 0.76, 0.26),
        "HARBOR - warm street luminaire {:02d}".format(index))


def add_signal(cube_mesh, mats, x, y, sign, index):
    box(cube_mesh, mats, "metal", (x, y, 520.0), (0.28, 0.28, 10.4),
        "HARBOR - signal pole {:02d}".format(index))
    box(cube_mesh, mats, "metal", (x, y, 1055.0), (0.45, 0.38, 1.25),
        "HARBOR - signal head {:02d}".format(index))
    for lamp, z in (("coral", 1125.0), ("amber", 1055.0), ("leaf", 985.0)):
        box(cube_mesh, mats, lamp, (x - sign * 36.0, y - 24.0, z), (0.12, 0.24, 0.22),
            "HARBOR - signal lens {:02d}".format(index))


def add_lighting(cube_mesh, mats):
    manager = unreal.EditorLevelLibrary.spawn_actor_from_class(
        native_class("GCDayNightManager"), unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
    )
    manager.set_editor_property("time_of_day", 17.35)
    manager.set_actor_label("LOOKDEV - warm late-afternoon sun and day-night rig", mark_dirty=True)

    sky_class = getattr(unreal, "SkyAtmosphere", None)
    if sky_class:
        sky = unreal.EditorLevelLibrary.spawn_actor_from_class(sky_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator())
        if sky:
            sky.set_actor_label("LOOKDEV - physical sky atmosphere", mark_dirty=True)

    fog_class = getattr(unreal, "ExponentialHeightFog", None)
    if fog_class:
        fog_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(fog_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator())
        fog_component_class = getattr(unreal, "ExponentialHeightFogComponent", None)
        if fog_actor and fog_component_class:
            fog = fog_actor.get_component_by_class(fog_component_class)
            safe_set(fog, "fog_density", 0.008)
            safe_set(fog, "fog_height_falloff", 0.16)
            safe_set(fog, "fog_in_scattering_color", unreal.LinearColor(0.42, 0.55, 0.60, 1.0))
            safe_set(fog, "b_enable_volumetric_fog", True)
            safe_set(fog, "volumetric_fog_scattering_distribution", 0.22)
            fog_actor.set_actor_label("LOOKDEV - coastal atmospheric depth", mark_dirty=True)

    pp_class = getattr(unreal, "PostProcessVolume", None)
    if pp_class:
        volume = unreal.EditorLevelLibrary.spawn_actor_from_class(pp_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator())
        if volume:
            safe_set(volume, "unbound", True)
            settings = volume.get_editor_property("settings")
            # API field names vary slightly across UE patch versions; only known fields are assigned.
            for prop, value in (
                ("bloom_intensity", 0.24), ("vignette_intensity", 0.28),
                ("film_grain_intensity", 0.08), ("ambient_occlusion_intensity", 0.75),
                ("color_saturation", unreal.Vector4(1.04, 1.02, 0.98, 1.0)),
                ("color_contrast", unreal.Vector4(1.06, 1.04, 1.02, 1.0)),
                ("auto_exposure_bias", 0.0),
            ):
                if safe_set(settings, prop, value):
                    safe_set(settings, "b_override_" + prop, True)
            safe_set(volume, "settings", settings)
            volume.set_actor_label("LOOKDEV - subtle filmic grade and restrained bloom", mark_dirty=True)


def main():
    if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
        raise RuntimeError("Could not create {}. Create an empty level and rerun.".format(MAP_PATH))

    cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    plane_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane.Plane")
    sphere_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Sphere.Sphere")
    if not cube_mesh or not plane_mesh or not sphere_mesh:
        raise RuntimeError("Required Engine/BasicShapes assets are unavailable.")
    mats = build_materials()

    # Shore district: asphalt boulevard, raised sidewalks, stone promenade, and the harbor edge.
    spawn_mesh(plane_mesh, (0.0, 0.0, -28.0), (1200.0, 1000.0, 1.0), mats["sidewalk"], "HARBOR - coastal district ground")
    box(cube_mesh, mats, "asphalt", (0.0, 0.0, -14.0), (30.0, 1000.0, 0.20), "HARBOR - Pacific Boulevard asphalt")
    for sign in (-1.0, 1.0):
        box(cube_mesh, mats, "white", (sign * 1540.0, 0.0, 4.0), (0.48, 1000.0, 0.15), "HARBOR - raised granite curb")
        box(cube_mesh, mats, "sidewalk", (sign * 2200.0, 0.0, 9.0), (12.2, 1000.0, 0.18), "HARBOR - scored concrete sidewalk")
        box(cube_mesh, mats, "limestone", (sign * 3300.0, 0.0, 7.0), (6.0, 1000.0, 0.13), "HARBOR - shopfront arcade walk")
        # Long inset expansion joints on the walk keep the broad surfaces from reading as slabs.
        for y in range(-48000, 48001, 2000):
            box(cube_mesh, mats, "trim", (sign * 2200.0, y, 19.0), (10.6, 0.05, 0.02), "HARBOR - sidewalk joint")

    # Lane line, broken center stripe, and six broad pedestrian crossings.
    box(cube_mesh, mats, "paint", (-950.0, 0.0, -2.0), (0.16, 1000.0, 0.035), "HARBOR - west lane edge")
    box(cube_mesh, mats, "paint", (950.0, 0.0, -2.0), (0.16, 1000.0, 0.035), "HARBOR - east lane edge")
    for y in range(-45000, 45001, 7200):
        box(cube_mesh, mats, "paint", (0.0, y, -1.0), (0.16, 3.0, 0.035), "HARBOR - broken center line")
    for crossing, y in enumerate((-42000.0, -25200.0, -8400.0, 8400.0, 25200.0, 42000.0)):
        for stripe in range(12):
            x = -1350.0 + stripe * 245.0
            box(cube_mesh, mats, "white", (x, y, 5.0), (0.86, 4.8, 0.045),
                "HARBOR - zebra crossing {:02d}".format(crossing))
        for sign in (-1.0, 1.0):
            box(cube_mesh, mats, "coral", (sign * 2120.0, y, 48.0), (0.8, 1.8, 0.03),
                "HARBOR - tactile crossing pad")

    # Original mixed-use waterfront facades: shop, hotel, market, cinema and office corner.
    facades = [
        ("limestone", "Cove House", 5, 7, 3300.0, 2700.0, 3000.0),
        ("coral", "Mariner Market", 4, 8, 3900.0, 3000.0, 2600.0),
        ("teal", "Harbor Hotel", 11, 7, 3600.0, 3000.0, 6400.0),
        ("stucco", "Pacific Cinema", 6, 8, 4300.0, 3100.0, 4100.0),
        ("limestone", "Tide Exchange", 8, 7, 4000.0, 3400.0, 5100.0),
        ("coral", "Old Wharf Lofts", 7, 8, 3600.0, 3000.0, 4600.0),
        ("teal", "Seabright Offices", 10, 8, 4500.0, 3300.0, 6100.0),
        ("stucco", "The Lantern", 5, 7, 3300.0, 2800.0, 3100.0),
    ]
    building_index = 0
    for sign in (-1.0, 1.0):
        for slot, y in enumerate((-42000.0, -30000.0, -18000.0, -6000.0, 6000.0, 18000.0, 30000.0, 42000.0)):
            palette, name, floors, bays, width, depth, height = facades[(slot + (1 if sign > 0 else 0)) % len(facades)]
            make_facade(cube_mesh, mats, sign, y, floors, bays, width, depth, height, palette,
                        "{:02d}_{}".format(building_index, name))
            building_index += 1

    # Palm promenade, benches, mast lights and marked junctions.
    palm_index = 0
    for sign in (-1.0, 1.0):
        for y in range(-42000, 42001, 8400):
            add_palm(cube_mesh, sphere_mesh, mats, sign * 2550.0, float(y), palm_index)
            palm_index += 1
        for index, y in enumerate(range(-42000, 42001, 12000)):
            add_street_light(cube_mesh, sphere_mesh, mats, sign * 1320.0, float(y), index + (0 if sign < 0 else 10))
    signal_index = 0
    for y in (-42000.0, -14000.0, 14000.0, 42000.0):
        for sign in (-1.0, 1.0):
            add_signal(cube_mesh, mats, sign * 1810.0, y, sign, signal_index)
            signal_index += 1

    # A modest fixed-resolution procedural grid deforms with the same analytic wave equations
    # used by gameplay buoyancy/query code. Tune extent and resolution after profiling in UE.
    box(cube_mesh, mats, "limestone", (9200.0, 0.0, 14.0), (30.0, 1000.0, 0.18), "HARBOR - public pier boardwalk")
    ocean = unreal.EditorLevelLibrary.spawn_actor_from_class(
        native_class("GCOceanManager"), unreal.Vector(39000.0, 0.0, 0.0), unreal.Rotator()
    )
    ocean.set_editor_property("b_generate_visual_surface", True)
    ocean.set_editor_property("visual_surface_half_extent", unreal.Vector2D(30000.0, 50000.0))
    ocean.set_editor_property("visual_surface_grid_resolution", 192)
    ocean.get_editor_property("wave_surface").set_material(0, mats["water_dynamic"])
    ocean.set_actor_label("HARBOR - animated analytic wave surface", mark_dirty=True)

    # Game systems and a late-afternoon third-person starting position facing down the boulevard.
    spawn(native_class("GCWorldSpawner"), (0.0, 0.0, 0.0))
    spawn(native_class("GCGrassFieldActor"), (7000.0, 10000.0, 0.0))
    spawn(native_class("GCMarineLifeActor"), (13000.0, 0.0, 0.0))
    spawn(native_class("GCComputerTerminalActor"), (-7200.0, 4200.0, 0.0))
    start = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0.0, -9200.0, 180.0), unreal.Rotator(0.0, 90.0, 0.0)
    )
    start.set_actor_label("PLAYER - Pacific Boulevard opening view", mark_dirty=True)
    vehicle = spawn(native_class("GCVehiclePawn"), (0.0, -8650.0, 100.0))
    vehicle.set_actor_label("PLAYER VEHICLE - assign imported Arden GT body if available", mark_dirty=True)
    vehicle_mesh = None
    for asset_path in (
        "/Game/GrandCity/Models/grandcity_arden_gt",
        "/Game/GrandCity/Models/babyloncar",
        "/Game/GrandCity/Models/groundvehicle",
    ):
        vehicle_mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
        if vehicle_mesh:
            break
    if vehicle_mesh:
        vehicle.get_editor_property("body_mesh").set_static_mesh(vehicle_mesh)
        unreal.log("Assigned vehicle body {}".format(vehicle_mesh.get_name()))
    else:
        unreal.log_warning("No imported vehicle mesh found. Run Tools/ImportModels_Editor.py first, then rerun this map builder.")
    add_lighting(cube_mesh, mats)

    # Keep the city population away from the open-water half of this test slice.
    spawner = next((a for a in unreal.EditorLevelLibrary.get_all_level_actors()
                    if a.get_class().get_name() == "GCWorldSpawner"), None)
    if spawner:
        safe_set(spawner, "spawn_min_radius", 2200.0)
        safe_set(spawner, "spawn_max_radius", 7600.0)
        safe_set(spawner, "despawn_radius", 11000.0)

    unreal.EditorLevelLibrary.save_current_level()
    unreal.EditorAssetLibrary.save_directory(MATERIAL_DIR, only_if_is_dirty=False)
    unreal.log("Built {}. This is a look-development vertical slice; run in UE and replace procedural kit geometry with authored assets.".format(MAP_PATH))


main()
