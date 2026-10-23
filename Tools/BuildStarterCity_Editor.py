"""Build the Gridlock starter city using imported community and original modeled assets.

Run from Unreal Editor: Tools > Execute Python Script. The builder imports required GLBs,
creates the starter map, and fails rather than substituting visible placeholder buildings.
Unreal runtime/art validation is still required after the editor script completes.
"""
import os
import re
import unreal

MAP_PATH = "/Game/Maps/L_TestCity"


def native_class(name):
    cls = unreal.load_class(None, "/Script/GrandCity." + name)
    if not cls:
        raise RuntimeError("Could not load native class {}; build the GrandCity Editor target first.".format(name))
    return cls


def spawn(cls, location, rotation=None):
    return unreal.EditorLevelLibrary.spawn_actor_from_class(
        cls, unreal.Vector(*location), rotation or unreal.Rotator(0.0, 0.0, 0.0)
    )


def enum_value(enum_names, value_name):
    enum_type = next((getattr(unreal, name) for name in enum_names if hasattr(unreal, name)), None)
    if enum_type is None:
        raise RuntimeError("Unreal Python did not expose enum {}".format(enum_names))
    expected = re.sub(r"[^A-Z0-9]", "", value_name.upper())
    for candidate in dir(enum_type):
        if re.sub(r"[^A-Z0-9]", "", candidate.upper()) == expected:
            return getattr(enum_type, candidate)
    raise RuntimeError("Enum value {} not found in {}".format(value_name, enum_type))


def import_hotel_ad_art():
    source_dir = os.path.join(unreal.Paths.project_dir(), "Resources", "HotelRewards")
    if not os.path.isdir(source_dir):
        unreal.log_warning("Hotel campaign art source folder was not found: {}".format(source_dir))
        return

    filenames = ("T_TidehouseHotels.png", "T_MeridianHouse.png", "T_AstaraGrand.png")
    tasks = []
    for filename in filenames:
        source = os.path.join(source_dir, filename)
        if not os.path.isfile(source):
            unreal.log_warning("Hotel campaign image is missing: {}".format(source))
            continue
        task = unreal.AssetImportTask()
        task.filename = source
        task.destination_path = "/Game/HotelRewards"
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)

    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
        unreal.EditorAssetLibrary.save_directory("/Game/HotelRewards", only_if_is_dirty=False)


def cube(mesh, location, scale, label, rotation=None):
    actor = spawn(unreal.StaticMeshActor, location, rotation)
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_actor_label(label, mark_dirty=True)
    return actor


def place_static_mesh(mesh, location, scale, label, rotation=None, collision=False):
    actor = spawn(unreal.StaticMeshActor, location, rotation)
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(mesh)
    component.set_collision_enabled(
        unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION
    )
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_actor_label(label, mark_dirty=True)
    return actor


def find_coastline_aircraft_mesh():
    """Resolve the original Coastline A320 mesh; caller fails instead of placing a cube."""
    candidates = (
        "/Game/GrandCity/Models/gridlock_coastline_a320.gridlock_coastline_a320",
        "/Game/GrandCity/Models/gridlock_coastline_a320.Mesh",
    )
    for path in candidates:
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if asset and isinstance(asset, unreal.StaticMesh):
            return asset
    try:
        registry = unreal.AssetRegistryHelpers.get_asset_registry()
        for asset_data in registry.get_assets_by_path("/Game/GrandCity/Models", recursive=True):
            name = str(asset_data.asset_name).lower()
            if "gridlock_coastline_a320" not in name:
                continue
            asset = asset_data.get_asset()
            if asset and isinstance(asset, unreal.StaticMesh):
                return asset
    except Exception as error:
        unreal.log_warning("Could not query the imported Coastline GLB: {}".format(error))
    return None


def assign_kenney_component(component, kit, model_name, target_dimensions):
    mesh = find_kenney_mesh(kit, model_name)
    if not component or not mesh:
        raise RuntimeError("Required community model {}/{} is not imported.".format(kit, model_name))
    component.set_static_mesh(mesh)
    scale = fit_mesh_scale(mesh, *target_dimensions)
    component.set_relative_scale3d(unreal.Vector(scale, scale, scale))
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)


def find_kenney_mesh(kit, model_name):
    destination_names = {"city-commercial": "CityCommercial", "city-suburban": "CitySuburban",
                         "city-industrial": "CityIndustrial", "city-roads": "CityRoads", "car": "Car",
                         "nature": "Nature", "watercraft": "Watercraft"}
    folder = destination_names.get(kit)
    if not folder:
        return None
    base = "/Game/GrandCity/Community/Kenney/{}/".format(folder)
    for path in (base + model_name + "." + model_name, base + model_name):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if asset and isinstance(asset, unreal.StaticMesh):
            return asset
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for asset_data in registry.get_assets_by_path(base.rstrip("/"), recursive=True):
        if str(asset_data.asset_name).lower() == model_name.lower():
            asset = asset_data.get_asset()
            if asset and isinstance(asset, unreal.StaticMesh):
                return asset
    return None


def find_gridlock_model(model_name):
    root = "/Game/GrandCity/Models/"
    for path in (root + model_name + "." + model_name, root + model_name):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if asset and isinstance(asset, unreal.StaticMesh):
            return asset
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for asset_data in registry.get_assets_by_path(root.rstrip("/"), recursive=True):
        if str(asset_data.asset_name).lower() == model_name.lower():
            asset = asset_data.get_asset()
            if asset and isinstance(asset, unreal.StaticMesh):
                return asset
    return None


def fit_mesh_scale(mesh, target_width_cm, target_depth_cm, target_height_cm):
    try:
        bounds = mesh.get_bounds()
        ext = bounds.box_extent
        dims = (2.0 * float(ext.x), 2.0 * float(ext.y), 2.0 * float(ext.z))
        if min(dims) > 0.01:
            return min(target_width_cm / dims[0], target_depth_cm / dims[1], target_height_cm / dims[2])
    except Exception as error:
        unreal.log_warning("Could not inspect {} bounds: {}".format(mesh.get_name(), error))
    return 10.0


def place_kenney_building(kit, model_name, location, label, target_height_cm=2500.0, yaw=0.0):
    mesh = find_kenney_mesh(kit, model_name)
    if not mesh:
        raise RuntimeError("Community model {}/{} is not imported; run Tools/ImportModels_Editor.py first.".format(kit, model_name))
    actor = spawn(unreal.StaticMeshActor, location, unreal.Rotator(0.0, yaw, 0.0))
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(mesh)
    component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    scale = fit_mesh_scale(mesh, 1750.0, 1450.0, target_height_cm)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    actor.set_actor_label(label, mark_dirty=True)
    return actor


def place_kenney_prop(kit, model_name, location, label, target_dimensions=(800.0, 800.0, 1200.0), yaw=0.0):
    mesh = find_kenney_mesh(kit, model_name)
    if not mesh:
        raise RuntimeError("Community prop {}/{} is not imported.".format(kit, model_name))
    actor = spawn(unreal.StaticMeshActor, location, unreal.Rotator(0.0, yaw, 0.0))
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(mesh)
    component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    scale = fit_mesh_scale(mesh, *target_dimensions)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    actor.set_actor_label(label, mark_dirty=True)
    return actor


def find_airport_terminal_mesh():
    return find_gridlock_model("gridlock_airport_terminal")


def build_airport_terminal(cube_mesh, airport_x, airport_y, airport_name, airport_code, destination_code, destination_city):
    """Place the original detailed terminal, a hidden walkable slab and the live airline kiosk."""
    cx, cy = airport_x, airport_y
    terminal_mesh = find_airport_terminal_mesh()
    if not terminal_mesh:
        raise RuntimeError("Modeled terminal is not imported; run Tools/ImportModels_Editor.py first.")
    terminal_actor = spawn(unreal.StaticMeshActor, (cx, cy, 0.0))
    terminal_component = terminal_actor.get_component_by_class(unreal.StaticMeshComponent)
    terminal_component.set_static_mesh(terminal_mesh)
    terminal_component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    terminal_actor.set_actor_label("{} - modeled terminal".format(airport_name), mark_dirty=True)
    floor = cube(cube_mesh, (cx, cy, -12.0), (34.0, 22.0, 0.18), airport_name + " - invisible floor collision")
    floor.set_actor_hidden_in_game(True)
    kiosk = spawn(native_class("GCComputerTerminalActor"), (cx - 1230.0, cy - 330.0, 0.0))
    kiosk.set_editor_property("airport_code", airport_code)
    kiosk.set_editor_property("airport_city", airport_name)
    kiosk.set_editor_property("destination_code", destination_code)
    kiosk.set_editor_property("destination_city", destination_city)
    kiosk.set_actor_label("{} - Coastline Airways self-service kiosk".format(airport_name), mark_dirty=True)
    return kiosk

def ensure_starter_models_imported():
    """Import the required original and Kenney community GLBs before creating map actors."""
    required = (find_gridlock_model("gridlock_airport_terminal"),
                find_gridlock_model("gridlock_pacific_residences"),
                find_gridlock_model("gridlock_escalator_step"),
                find_gridlock_model("gridlock_escalator_railings"),
                find_gridlock_model("gridlock_elevator_guide_frame"),
                find_gridlock_model("gridlock_surfboard"),
                find_kenney_mesh("city-commercial", "building-a"),
                find_kenney_mesh("city-suburban", "building-type-b"),
                find_kenney_mesh("city-industrial", "building-a"),
                find_kenney_mesh("city-roads", "road-straight"),
                find_kenney_mesh("city-roads", "road-crossroad"),
                find_kenney_mesh("city-roads", "light-square"),
                find_kenney_mesh("city-suburban", "tree-large"),
                find_kenney_mesh("car", "van"),
                find_kenney_mesh("car", "sedan"),
                find_kenney_mesh("nature", "sign"),
                find_kenney_mesh("watercraft", "boat-speed-a"),
                find_gridlock_model("helicopter"))
    if all(required):
        return
    project_dir = unreal.Paths.project_dir()
    groups = (
        (os.path.join(project_dir, "ModelLibrary", "PublicGLB"), "/Game/GrandCity/Models"),
        (os.path.join(project_dir, "ModelLibrary", "Community", "Kenney", "city-commercial"), "/Game/GrandCity/Community/Kenney/CityCommercial"),
        (os.path.join(project_dir, "ModelLibrary", "Community", "Kenney", "city-suburban"), "/Game/GrandCity/Community/Kenney/CitySuburban"),
        (os.path.join(project_dir, "ModelLibrary", "Community", "Kenney", "city-industrial"), "/Game/GrandCity/Community/Kenney/CityIndustrial"),
        (os.path.join(project_dir, "ModelLibrary", "Community", "Kenney", "city-roads"), "/Game/GrandCity/Community/Kenney/CityRoads"),
        (os.path.join(project_dir, "ModelLibrary", "Community", "Kenney", "car"), "/Game/GrandCity/Community/Kenney/Car"),
        (os.path.join(project_dir, "ModelLibrary", "Community", "Kenney", "nature"), "/Game/GrandCity/Community/Kenney/Nature"),
        (os.path.join(project_dir, "ModelLibrary", "Community", "Kenney", "watercraft"), "/Game/GrandCity/Community/Kenney/Watercraft"),
        (os.path.join(project_dir, "ModelLibrary", "Community", "FlightAviation"), "/Game/GrandCity/Models"),
    )
    for source_dir, destination in groups:
        unreal.EditorAssetLibrary.make_directory(destination)
        tasks = []
        for root, _, filenames in os.walk(source_dir):
            for filename in sorted(filenames):
                if not filename.lower().endswith(".glb"):
                    continue
                task = unreal.AssetImportTask()
                task.filename = os.path.join(root, filename)
                task.destination_path = destination
                task.automated = True
                task.replace_existing = True
                task.save = True
                tasks.append(task)
        if tasks:
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
            if not any(task.imported_object_paths for task in tasks):
                raise RuntimeError("Unreal imported no GLBs from {}. Enable the GLTF importer plugin.".format(source_dir))
        unreal.EditorAssetLibrary.save_directory(destination, only_if_is_dirty=False)
    post_import = (find_gridlock_model("gridlock_airport_terminal"),
                   find_gridlock_model("gridlock_escalator_step"),
                   find_gridlock_model("gridlock_escalator_railings"),
                   find_gridlock_model("gridlock_elevator_guide_frame"),
                   find_gridlock_model("gridlock_surfboard"),
                   find_kenney_mesh("city-commercial", "building-a"),
                   find_kenney_mesh("city-suburban", "building-type-b"),
                   find_kenney_mesh("city-industrial", "building-a"),
                   find_kenney_mesh("city-roads", "road-straight"),
                   find_kenney_mesh("city-roads", "road-crossroad"),
                   find_kenney_mesh("city-roads", "light-square"),
                   find_kenney_mesh("city-suburban", "tree-large"),
                   find_kenney_mesh("car", "van"),
                   find_kenney_mesh("car", "sedan"),
                   find_kenney_mesh("nature", "sign"),
                   find_kenney_mesh("watercraft", "boat-speed-a"),
                   find_gridlock_model("helicopter"))
    if not all(post_import):
        raise RuntimeError("Required GLBs could not be resolved after import; inspect Unreal import warnings.")


def create_city_material(name, color, roughness, metallic, vertex_foam=False):
    folder = "/Game/GrandCity/Materials/StarterCity"
    unreal.EditorAssetLibrary.make_directory(folder)
    path = folder + "/M_" + name
    existing = unreal.EditorAssetLibrary.load_asset(path)
    if existing:
        return existing
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_" + name, folder, unreal.Material, unreal.MaterialFactoryNew()
    )
    if not material:
        raise RuntimeError("Could not create starter city material {}".format(name))
    graph = unreal.MaterialEditingLibrary
    base = graph.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -420, -60)
    base.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    rough = graph.create_material_expression(material, unreal.MaterialExpressionConstant, -420, 110)
    rough.set_editor_property("r", roughness)
    metal = graph.create_material_expression(material, unreal.MaterialExpressionConstant, -420, 230)
    metal.set_editor_property("r", metallic)
    if vertex_foam:
        vertex = graph.create_material_expression(material, unreal.MaterialExpressionVertexColor, -180, 330)
        foam = graph.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -180, 20)
        foam.set_editor_property("constant", unreal.LinearColor(0.82, 0.93, 0.94, 1.0))
        blend = graph.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, 100, -50)
        graph.connect_material_expressions(base, "", blend, "A")
        graph.connect_material_expressions(foam, "", blend, "B")
        graph.connect_material_expressions(vertex, "R", blend, "Alpha")
        graph.connect_material_property(blend, "", unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        graph.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    graph.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    graph.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    graph.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def main():
    ensure_starter_models_imported()
    if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
        raise RuntimeError("Could not create {}. Create an empty level manually and rerun.".format(MAP_PATH))

    import_hotel_ad_art()

    cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    plane_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane.Plane")
    if not cube_mesh or not plane_mesh:
        raise RuntimeError("Unreal BasicShapes assets are unavailable; restore Engine/BasicShapes.")

    # A flat, material-finished island platform with a live procedural ocean outside its edge.
    ground = cube(plane_mesh, (0.0, 0.0, -80.0), (200.0, 200.0, 1.0), "Gridlock island landscape base")
    ground_component = ground.get_component_by_class(unreal.StaticMeshComponent)
    ground_component.set_material(0, create_city_material("IslandGrass", (0.16, 0.25, 0.12), 0.92, 0.0))
    island_collision = cube(cube_mesh, (0.0, 0.0, -90.0), (100.0, 100.0, 0.1), "Island underside - hidden collision")
    island_collision.set_actor_hidden_in_game(True)
    ocean = spawn(native_class("GCOceanManager"), (0.0, 0.0, -150.0))
    ocean.set_editor_property("b_generate_visual_surface", True)
    ocean.set_editor_property("visual_surface_half_extent", unreal.Vector2D(30000.0, 30000.0))
    ocean.set_editor_property("visual_surface_grid_resolution", 128)
    ocean.get_editor_property("wave_surface").set_material(
        0, create_city_material("StarterOcean", (0.018, 0.105, 0.135), 0.16, 0.18, vertex_foam=True)
    )
    ocean.set_actor_label("Animated analytic ocean - Gridlock island", mark_dirty=True)

    # Tile the CC0 Kenney street kit at real block scale; the old stretched plane strips are gone.
    road_mesh = find_kenney_mesh("city-roads", "road-straight")
    crossroad_mesh = find_kenney_mesh("city-roads", "road-crossroad")
    if not road_mesh or not crossroad_mesh:
        raise RuntimeError("Kenney City Kit Roads is not imported; run Tools/ImportModels_Editor.py first.")
    road_slots = tuple(float(i * 2000) for i in range(-4, 5))
    tile_centers = tuple(float(-5000 + i * 1000) for i in range(11))
    for x in road_slots:
        for y in tile_centers:
            if x == 0.0 and y == 0.0:
                continue
            place_static_mesh(road_mesh, (x, y, -24.0), (5.5, 10.0, 1.0), "Kenney CC0 - north/south road tile")
            collider = cube(cube_mesh, (x, y, -27.0), (2.75, 5.0, 0.03), "Road collision - north/south")
            collider.set_actor_hidden_in_game(True)
    for y in road_slots:
        for x in tile_centers:
            if y == 0.0 and x == 0.0:
                continue
            place_static_mesh(road_mesh, (x, y, -23.0), (5.5, 10.0, 1.0),
                              "Kenney CC0 - east/west road tile", unreal.Rotator(0.0, 90.0, 0.0))
            collider = cube(cube_mesh, (x, y, -26.0), (5.0, 2.75, 0.03), "Road collision - east/west")
            collider.set_actor_hidden_in_game(True)
    place_static_mesh(crossroad_mesh, (0.0, 0.0, -22.0), (5.5, 5.5, 1.0),
                      "Kenney CC0 - downtown four-way junction")
    junction_collision = cube(cube_mesh, (0.0, 0.0, -25.0), (2.75, 2.75, 0.03), "Junction collision")
    junction_collision.set_actor_hidden_in_game(True)

    # Fifteen CC0 modular storefronts plus one original, high-detail residential landmark.
    # The landmark replaces the former simple community skyscraper at the outer downtown block.
    building_assets = ["building-{}".format(letter) for letter in "abcdefghijklmn"]
    building_assets.append("building-skyscraper-a")
    block_positions = (-5000.0, -1000.0, 1000.0, 5000.0)
    for index, model_name in enumerate(building_assets):
        x, y = block_positions[index % 4], block_positions[index // 4]
        tall = "skyscraper" in model_name
        place_kenney_building("city-commercial", model_name, (x, y, 0.0),
                              "Gridlock District - {}".format(model_name.replace("-", " ").title()),
                              target_height_cm=5400.0 if tall else 2600.0, yaw=(index % 4) * 90.0)
    pacific_mesh = find_gridlock_model("gridlock_pacific_residences")
    if not pacific_mesh:
        raise RuntimeError("Original Pacific Residences GLB is not imported.")
    pacific_actor = spawn(unreal.StaticMeshActor, (5000.0, 5000.0, 40.0))
    pacific_component = pacific_actor.get_component_by_class(unreal.StaticMeshComponent)
    pacific_component.set_static_mesh(pacific_mesh)
    pacific_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    pacific_scale = fit_mesh_scale(pacific_mesh, 5250.0, 3500.0, 6700.0)
    pacific_actor.set_actor_scale3d(unreal.Vector(pacific_scale, pacific_scale, pacific_scale))
    pacific_actor.set_actor_label("Gridlock Original - Pacific Residences landmark", mark_dirty=True)
    city_collision = cube(cube_mesh, (5000.0, 5000.0, 3300.0), (52.0, 34.0, 66.0),
                          "Pacific Residences - hidden simple building collision")
    city_collision.set_actor_hidden_in_game(True)

    # CC0 suburban trees and road lamps give the city blocks a finished streetscape.
    tree_positions = ((-7500.0,-4000.0),(-7500.0,0.0),(-7500.0,4000.0),(7500.0,-4000.0),
                      (7500.0,0.0),(7500.0,4000.0),(-4000.0,7500.0),(0.0,7500.0),(4000.0,7500.0),
                      (-4000.0,-7500.0),(0.0,-7500.0),(4000.0,-7500.0))
    for index, (x,y) in enumerate(tree_positions):
        tree = "tree-large" if index % 3 else "tree-small"
        place_kenney_prop("city-suburban", tree, (x,y,-75.0), "Kenney CC0 - landscaped street tree {:02d}".format(index+1),
                          (650.0,650.0,1050.0), yaw=(index*37)%360)
    lamp_mesh = find_kenney_mesh("city-roads", "light-square")
    if not lamp_mesh:
        raise RuntimeError("Kenney square streetlight is not imported.")
    for index, (x,y) in enumerate(((-7000.0,-7000.0),(-1000.0,-7000.0),(5000.0,-7000.0),
                                   (-7000.0,7000.0),(1000.0,7000.0),(7000.0,0.0))):
        place_kenney_prop("city-roads", "light-square", (x,y,-75.0),
                          "Kenney CC0 - street lamp {:02d}".format(index+1), (160.0,160.0,750.0))

    spawn(unreal.PlayerStart, (0.0, -400.0, 180.0))
    spawn(native_class("GCDayNightManager"), (0.0, 0.0, 0.0))
    spawn(native_class("GCWorldSpawner"), (0.0, 0.0, 0.0))
    spawn(native_class("GCGrassFieldActor"), (900.0, 900.0, 0.0))
    spawn(native_class("GCMarineLifeActor"), (0.0, 12000.0, 0.0))
    spawn(native_class("GCComputerTerminalActor"), (6500.0, -6500.0, 0.0))

    # Interactable POIs use distinct CC0 modeled buildings. Link airports for ticketed flights.
    location_cls = native_class("GCLocationActor")
    locations = []
    for enum_name, label, location in (
        ("Hospital", "TEST - hospital", (-6500.0, -6500.0, 0.0)),
        ("Shop", "TEST - shop", (-2500.0, -2500.0, 0.0)),
        ("PoliceStation", "TEST - police station", (2500.0, -2500.0, 0.0)),
        ("Airport", "TEST - airport west", (-7500.0, 6500.0, 0.0)),
        ("Airport", "TEST - airport east", (7500.0, 6500.0, 0.0)),
        ("Marina", "TEST - marina", (0.0, 10000.0, 0.0)),
        ("SkyscraperElevator", "TEST - elevator", (6500.0, 2500.0, 0.0)),
    ):
        actor = spawn(location_cls, location)
        actor.set_editor_property(
            "location_type", enum_value(("EGCLocationType", "GCLocationType"), enum_name)
        )
        actor.set_actor_label(label, mark_dirty=True)
        mesh_component = actor.get_editor_property("building_mesh")
        if mesh_component:
            if enum_name in ("SkyscraperElevator", "Airport"):
                mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
                mesh_component.set_visibility(False, True)
            else:
                poi_asset = {
                    "Hospital": ("city-commercial", "building-n", 3000.0),
                    "Shop": ("city-commercial", "building-a", 1200.0),
                    "PoliceStation": ("city-commercial", "building-h", 1800.0),
                    "Marina": ("city-industrial", "building-a", 1400.0),
                }.get(enum_name)
                if not poi_asset:
                    raise RuntimeError("No community model assigned for POI {}".format(enum_name))
                poi_mesh = find_kenney_mesh(poi_asset[0], poi_asset[1])
                if not poi_mesh:
                    raise RuntimeError("POI community model {}/{} is not imported.".format(poi_asset[0], poi_asset[1]))
                mesh_component.set_static_mesh(poi_mesh)
                scale = fit_mesh_scale(poi_mesh, 1800.0, 1500.0, poi_asset[2])
                mesh_component.set_relative_scale3d(unreal.Vector(scale, scale, scale))
                mesh_component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
        locations.append(actor)
    locations[3].set_editor_property("flight_destination", locations[4])
    locations[4].set_editor_property("flight_destination", locations[3])

    # Two walkable modeled terminals with separate Coastline Airways kiosks and reciprocal flights.
    build_airport_terminal(cube_mesh, -8000.0, 6500.0, "Gridlock International", "GCI", "NIS", "North Island")
    build_airport_terminal(cube_mesh, 8000.0, 6500.0, "North Island Airport", "NIS", "GCI", "Gridlock")

    # Reciprocal passenger services use the linked airport actors as route endpoints.
    airliner_cls = native_class("GCAirlinerPawn")
    coastline_mesh = find_coastline_aircraft_mesh()
    if not coastline_mesh:
        raise RuntimeError("Coastline aircraft GLB is not imported; run Tools/ImportModels_Editor.py first.")
    for airport_x, airport_y, origin_code, destination_code, destination_actor, registration in (
        (-8000.0, 8200.0, "GCI", "NIS", locations[4], "PM-041"),
        (8000.0, 8200.0, "NIS", "GCI", locations[3], "PM-042"),
    ):
        airliner = spawn(airliner_cls, (airport_x, airport_y, 100.0))
        airliner.set_editor_property("airline_name", "Coastline Airways")
        airliner.set_editor_property("flight_number", "CW 410")
        airliner.set_editor_property("aircraft_registration", registration)
        airliner.set_editor_property("from_code", origin_code)
        airliner.set_editor_property("to_code", destination_code)
        airliner.set_editor_property("destination_actor", destination_actor)
        airliner.set_editor_property("b_auto_deploy_jetway", True)
        airliner.set_editor_property("b_require_checked_in_ticket", True)
        airliner.set_actor_label("TEST - Coastline Airways {} service {}".format(origin_code, registration), mark_dirty=True)
        fuselage = airliner.get_editor_property("fuselage_mesh")
        if not fuselage:
            raise RuntimeError("GCAirlinerPawn is missing its fuselage mesh component.")
        fuselage.set_static_mesh(coastline_mesh)
        fuselage.set_relative_scale3d(unreal.Vector(1.0, 1.0, 1.0))

    # A real moving car between the lobby and roof stations, with an open, visible guide shaft.
    elevator_cls = native_class("GCElevatorActor")
    elevator = spawn(elevator_cls, (6500.0, 2500.0, 0.0))
    elevator.set_actor_label("TEST - moving tower elevator car", mark_dirty=True)
    base_landing = locations[6]
    roof_landing = spawn(location_cls, (6500.0, 2500.0, 3000.0))
    roof_landing.set_editor_property("location_type", enum_value(("EGCLocationType", "GCLocationType"), "SkyscraperElevator"))
    roof_landing.set_actor_label("TEST - tower roof elevator landing", mark_dirty=True)
    roof_mesh = roof_landing.get_editor_property("building_mesh")
    if roof_mesh:
        roof_mesh.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        roof_mesh.set_visibility(False, True)
    for landing, destination in ((base_landing, roof_landing), (roof_landing, base_landing)):
        landing.set_editor_property("teleport_destination", destination)
        landing.set_editor_property("elevator_actor", elevator)

    guide_mesh = find_gridlock_model("gridlock_elevator_guide_frame")
    if not guide_mesh:
        raise RuntimeError("Elevator guide-frame GLB is not imported; run Tools/ImportModels_Editor.py first.")
    guide_actor = place_static_mesh(guide_mesh, (6500.0, 2500.0, 0.0), (1.0, 1.0, 1.0),
                                    "Elevator - modeled structural guide frame, cross-bracing and access ladder")

    escalator_cls = native_class("GCEscalatorActor")
    escalator_mesh = find_gridlock_model("gridlock_escalator_step")
    railings_mesh = find_gridlock_model("gridlock_escalator_railings")
    if not escalator_mesh or not railings_mesh:
        raise RuntimeError("Detailed escalator GLBs are not imported; run Tools/ImportModels_Editor.py first.")
    for airport_x, label in ((-8000.0, "West terminal escalator"), (8000.0, "East terminal escalator")):
        rotation = unreal.Rotator(0.0, 90.0, 0.0)
        escalator = spawn(escalator_cls, (airport_x, 5900.0, 0.0), rotation)
        escalator.set_editor_property("tread_mesh", cube_mesh)  # collision proxy remains convex and dependable
        escalator.set_editor_property("tread_detail_mesh", escalator_mesh)
        escalator.set_editor_property("ramp_run_cm", 700.0)
        escalator.set_editor_property("ramp_rise_cm", 350.0)
        escalator.set_editor_property("width_cm", 190.0)
        escalator.set_editor_property("step_count", 28)
        escalator.set_actor_label(label + " - moving ribbed treads", mark_dirty=True)
        rails_actor = place_static_mesh(railings_mesh, (airport_x, 5900.0, 0.0), (1.0, 1.0, 1.0),
                                        label + " - glass guards, skirts and stainless handrails", rotation)

    # Small first-pass housing/electronics slice: buy a deed from the live realtor,
    # or seize the listing after killing the agent (wanted stars apply); Tab/E browse/buy devices.
    realtor = spawn(native_class("GCPedestrian"), (-3400.0, -7300.0, 100.0))
    realtor.set_actor_label("TEST - real estate agent", mark_dirty=True)
    listing = spawn(native_class("GCRealEstateActor"), (-3000.0, -7500.0, 0.0))
    listing.set_editor_property("property_id", "TEST-PACIFIC-HOUSE")
    listing.set_editor_property("property_name", "Pacific View House")
    listing.set_editor_property("purchase_price", 125000)
    listing.set_editor_property("realtor", realtor)
    sign = listing.get_editor_property("sign_mesh")
    assign_kenney_component(sign, "nature", "sign", (230.0, 140.0, 220.0))
    place_kenney_building("city-suburban", "building-type-b", (-3000.0, -7500.0, 0.0),
                          "Pacific View House - Kenney CC0 residential model", target_height_cm=1400.0)

    # Paired deed-checked access points demonstrate entry/exit for either purchased or stolen deeds.
    target_point_cls = unreal.load_class(None, "/Script/Engine.TargetPoint")
    if not target_point_cls:
        raise RuntimeError("Could not load Engine.TargetPoint for property door destinations.")
    outside_marker = spawn(target_point_cls, (-2200.0, -7500.0, 100.0))
    interior_marker = spawn(target_point_cls, (-4000.0, -7500.0, 100.0))
    outside_marker.set_actor_label("TEST - Pacific View outside arrival", mark_dirty=True)
    interior_marker.set_actor_label("TEST - Pacific View interior arrival", mark_dirty=True)
    property_door_cls = native_class("GCPropertyDoorActor")
    entrance = spawn(property_door_cls, (-2300.0, -7500.0, 0.0))
    entrance.set_editor_property("property_id", "TEST-PACIFIC-HOUSE")
    entrance.set_editor_property("door_name", "Pacific View House")
    entrance.set_editor_property("destination_marker", interior_marker)
    entrance.set_editor_property("b_entering_interior", True)
    entrance.set_actor_label("TEST - Pacific View deed-checked entrance", mark_dirty=True)
    exit_door = spawn(property_door_cls, (-4000.0, -7500.0, 0.0))
    exit_door.set_editor_property("property_id", "TEST-PACIFIC-HOUSE")
    exit_door.set_editor_property("door_name", "Pacific View House")
    exit_door.set_editor_property("destination_marker", outside_marker)
    exit_door.set_editor_property("b_entering_interior", False)
    exit_door.set_actor_label("TEST - Pacific View deed-checked exit", mark_dirty=True)

    device_store = spawn(native_class("GCDeviceStoreActor"), (4200.0, -5000.0, 0.0))
    device_store.set_actor_label("TEST - electronics retailer", mark_dirty=True)
    store_sign = device_store.get_editor_property("store_sign")
    assign_kenney_component(store_sign, "nature", "sign", (320.0, 160.0, 250.0))

    # Prototype bus routes use linked stops and the selected wallet payment method.
    bus_stop_cls = native_class("GCBusStopActor")
    downtown_stop = spawn(bus_stop_cls, (-1200.0, 1200.0, 0.0))
    west_stop = spawn(bus_stop_cls, (-8000.0, 5200.0, 0.0))
    east_stop = spawn(bus_stop_cls, (8000.0, 5200.0, 0.0))
    stops = ((downtown_stop, "Downtown Central"), (west_stop, "West Terminal"), (east_stop, "East Terminal"))
    for stop, stop_name in stops:
        stop.set_editor_property("stop_name", stop_name)
        stop.set_actor_label("TEST - bus stop - " + stop_name, mark_dirty=True)
        sign_mesh = stop.get_editor_property("stop_sign")
        assign_kenney_component(sign_mesh, "nature", "sign", (140.0, 140.0, 260.0))
    downtown_stop.set_editor_property("destination_stops", [west_stop, east_stop])
    west_stop.set_editor_property("destination_stops", [downtown_stop, east_stop])
    east_stop.set_editor_property("destination_stops", [downtown_stop, west_stop])

    # Route shuttle uses a Kenney CC0 vehicle visual; native bus collision, route and fares remain unchanged.
    van_mesh = find_kenney_mesh("car", "van")
    if not van_mesh:
        raise RuntimeError("Kenney CC0 van model is not imported.")
    bus = spawn(native_class("GCBusPawn"), (-650.0, 1200.0, 120.0))
    bus.set_actor_label("Gridlock Transit - community-model airport shuttle", mark_dirty=True)
    bus.set_editor_property("route_stops", [downtown_stop, west_stop, east_stop])
    bus_body = bus.get_editor_property("body_mesh")
    if not bus_body:
        raise RuntimeError("GCBusPawn is missing its body mesh component.")
    bus_body.set_static_mesh(van_mesh)
    bus_scale = fit_mesh_scale(van_mesh, 760.0, 270.0, 290.0)
    bus_body.set_relative_scale3d(unreal.Vector(bus_scale, bus_scale, bus_scale))
    bus_body.set_relative_location(unreal.Vector(0.0, 0.0, -145.0))

    # Each test vehicle keeps its native collision, handling, and interaction proxy;
    # the visible meshes are complete community or original game models.
    sedan_mesh = find_kenney_mesh("car", "sedan")
    car = spawn(native_class("GCVehiclePawn"), (300.0, 0.0, 120.0))
    car.set_actor_label("Personal vehicle - Kenney CC0 sedan", mark_dirty=True)
    car_body = car.get_editor_property("body_mesh")
    if not car_body or not sedan_mesh:
        raise RuntimeError("Vehicle body or Kenney sedan mesh is missing.")
    car_body.set_static_mesh(sedan_mesh)
    car_scale = fit_mesh_scale(sedan_mesh, 450.0, 190.0, 145.0)
    car_body.set_relative_scale3d(unreal.Vector(car_scale, car_scale, car_scale))
    car_body.set_relative_location(unreal.Vector(0.0, 0.0, -65.0))

    boat_mesh = find_kenney_mesh("watercraft", "boat-speed-a")
    boat = spawn(native_class("GCBoatPawn"), (0.0, 10500.0, -120.0))
    boat.set_actor_label("Watercraft test - Kenney CC0 speedboat", mark_dirty=True)
    hull = boat.get_editor_property("hull_mesh")
    if not hull or not boat_mesh:
        raise RuntimeError("Boat hull component or Kenney speedboat model is missing.")
    hull.set_static_mesh(boat_mesh)
    boat_scale = fit_mesh_scale(boat_mesh, 700.0, 250.0, 260.0)
    hull.set_relative_scale3d(unreal.Vector(boat_scale, boat_scale, boat_scale))
    hull.set_relative_location(unreal.Vector(0.0, 0.0, -55.0))

    plane = spawn(native_class("GCPlanePawn"), (-3500.0, 7000.0, 180.0))
    plane.set_actor_label("Flight test - Gridlock Coastline A320", mark_dirty=True)
    plane_body = plane.get_editor_property("fuselage_mesh")
    propeller = plane.get_editor_property("propeller_mesh")
    if not plane_body or not coastline_mesh:
        raise RuntimeError("Plane fuselage or Gridlock Coastline A320 model is missing.")
    plane_body.set_static_mesh(coastline_mesh)
    plane_scale = fit_mesh_scale(coastline_mesh, 3600.0, 3400.0, 1100.0)
    plane_body.set_relative_scale3d(unreal.Vector(plane_scale, plane_scale, plane_scale))
    plane_body.set_relative_location(unreal.Vector(0.0, 0.0, -110.0))
    if propeller:
        propeller.set_visibility(False, True)

    surfboard_mesh = find_gridlock_model("gridlock_surfboard")
    surfboard = spawn(native_class("GCSurfboardPawn"), (600.0, 10500.0, 120.0))
    surfboard.set_actor_label("Surf test - modeled Gridlock surfboard", mark_dirty=True)
    board_component = surfboard.get_editor_property("board_mesh")
    if not board_component or not surfboard_mesh:
        raise RuntimeError("Surfboard component or generated board model is missing.")
    board_component.set_static_mesh(surfboard_mesh)
    board_scale = fit_mesh_scale(surfboard_mesh, 260.0, 58.0, 20.0)
    board_component.set_relative_scale3d(unreal.Vector(board_scale, board_scale, board_scale))
    board_component.set_relative_location(unreal.Vector(0.0, 0.0, -8.0))

    helicopter_mesh = find_gridlock_model("helicopter")
    if not helicopter_mesh:
        raise RuntimeError("CC BY 4.0 helicopter model is not imported.")
    helicopter = spawn(native_class("GCHelicopterPawn"), (4200.0, 8000.0, 260.0))
    helicopter.set_actor_label("Gridlock Air Services - AW101 helicopter", mark_dirty=True)
    fuselage = helicopter.get_editor_property("fuselage_mesh")
    rotor = helicopter.get_editor_property("propeller_mesh")
    if not fuselage:
        raise RuntimeError("GCHelicopterPawn is missing its fuselage mesh component.")
    fuselage.set_static_mesh(helicopter_mesh)
    helicopter_scale = fit_mesh_scale(helicopter_mesh, 1500.0, 900.0, 650.0)
    fuselage.set_relative_scale3d(unreal.Vector(helicopter_scale, helicopter_scale, helicopter_scale))
    try:
        bounds = helicopter_mesh.get_bounds()
        fuselage.set_relative_location(unreal.Vector(0.0, 0.0, -20.0 - float(bounds.origin.z) * helicopter_scale))
    except Exception:
        fuselage.set_relative_location(unreal.Vector(0.0, 0.0, -20.0))
    fuselage.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    # The imported AW101 GLB contains its own rotor geometry; hide the prototype cube rotor component.
    if rotor:
        rotor.set_visibility(False, True)

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("Created {}. This is a greybox test map, not finished gameplay.".format(MAP_PATH))


main()
