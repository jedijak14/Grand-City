"""Create a simple editor-only greybox test map for the Grand City C++ systems.

Run from Unreal Editor: Tools > Execute Python Script. This is a test layout, not
finished art or evidence of gameplay. Save the generated map after running it.
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


def import_hotel_reward_logos():
    source_dir = os.path.join(unreal.Paths.project_dir(), "Resources", "HotelRewards")
    if not os.path.isdir(source_dir):
        unreal.log_warning("Hotel reward logo source folder was not found: {}".format(source_dir))
        return

    filenames = ("T_HiltonHonors.jpg", "T_MarriottBonvoy.jpg", "T_IHGOneRewards.png")
    tasks = []
    for filename in filenames:
        source = os.path.join(source_dir, filename)
        if not os.path.isfile(source):
            unreal.log_warning("Hotel reward logo is missing: {}".format(source))
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


def main():
    if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
        raise RuntimeError("Could not create {}. Create an empty level manually and rerun.".format(MAP_PATH))

    import_hotel_reward_logos()

    cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    plane_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane.Plane")
    if not cube_mesh or not plane_mesh:
        raise RuntimeError("Unreal BasicShapes assets are unavailable; restore Engine/BasicShapes.")

    # 20 km square ground plane and a 10 km square water surface outside the island.
    ground = cube(plane_mesh, (0.0, 0.0, -80.0), (200.0, 200.0, 1.0), "TEST - greybox island ground")
    ground.set_actor_label("TEST - greybox island ground", mark_dirty=True)
    cube(plane_mesh, (0.0, 23000.0, -15.0), (90.0, 90.0, 1.0), "TEST - flat water surface")

    # Main paved grid; visual cubes are intentionally only test geometry.
    for i in range(-4, 5):
        cube(cube_mesh, (i * 2000.0, 0.0, -30.0), (5.5, 110.0, 0.35), "TEST - north-south road")
        cube(cube_mesh, (0.0, i * 2000.0, -30.0), (110.0, 5.5, 0.35), "TEST - east-west road")
    for x in (-6000.0, -2000.0, 2000.0, 6000.0):
        for y in (-6000.0, -2000.0, 2000.0, 6000.0):
            cube(cube_mesh, (x, y, 380.0), (13.0, 13.0, 8.0), "TEST - placeholder building")

    spawn(unreal.PlayerStart, (0.0, -400.0, 180.0))
    spawn(native_class("GCDayNightManager"), (0.0, 0.0, 0.0))
    spawn(native_class("GCOceanManager"), (0.0, 0.0, 0.0))
    spawn(native_class("GCWorldSpawner"), (0.0, 0.0, 0.0))
    spawn(native_class("GCGrassFieldActor"), (900.0, 900.0, 0.0))
    spawn(native_class("GCMarineLifeActor"), (0.0, 12000.0, 0.0))
    spawn(native_class("GCComputerTerminalActor"), (6500.0, -6500.0, 0.0))

    # Interactable points of interest. Link the two airport actors so ticketed flights
    # have a destination; all buildings remain clearly labeled test geometry.
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
            if enum_name == "SkyscraperElevator":
                mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
                mesh_component.set_visibility(False, True)
            else:
                mesh_component.set_static_mesh(cube_mesh)
                mesh_component.set_relative_scale3d(unreal.Vector(8.0, 8.0, 7.0))
                mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        locations.append(actor)
    locations[3].set_editor_property("flight_destination", locations[4])
    locations[4].set_editor_property("flight_destination", locations[3])

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

    # Tall guide posts make the motion readable while leaving the cabin visible for testing.
    for side_x in (-165.0, 165.0):
        for side_y in (-165.0, 165.0):
            cube(cube_mesh, (6500.0 + side_x, 2500.0 + side_y, 1500.0),
                 (0.12, 0.12, 30.0), "TEST - elevator guide post")

    # Working moving-tread escalators beside the airport test points. Rails are simple greybox dressing.
    escalator_cls = native_class("GCEscalatorActor")
    escalator_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    for airport_x, label in ((-8000.0, "TEST - west terminal escalator"), (8000.0, "TEST - east terminal escalator")):
        escalator = spawn(escalator_cls, (airport_x, 5900.0, 0.0), unreal.Rotator(0.0, 90.0, 0.0))
        escalator.set_editor_property("tread_mesh", escalator_mesh)
        escalator.set_editor_property("ramp_run_cm", 700.0)
        escalator.set_editor_property("ramp_rise_cm", 350.0)
        escalator.set_editor_property("width_cm", 190.0)
        escalator.set_editor_property("step_count", 28)
        escalator.set_actor_label(label, mark_dirty=True)
        rail_rotation = unreal.Rotator(26.565, 90.0, 0.0)
        for side in (-1.0, 1.0):
            cube(escalator_mesh, (airport_x + side * 135.0, 6250.0, 275.0),
                 (4.0, 0.06, 0.06), label + " handrail", rail_rotation)

    # Add visible default vehicle test pawns. Custom Blueprint children and imported
    # models should replace these defaults when available.
    for name, location in (
        ("GCVehiclePawn", (300.0, 0.0, 120.0)),
        ("GCBoatPawn", (0.0, 10500.0, 120.0)),
        ("GCPlanePawn", (-3500.0, 7000.0, 180.0)),
        ("GCSurfboardPawn", (600.0, 10500.0, 120.0)),
    ):
        spawn(native_class(name), location)

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("Created {}. This is a greybox test map, not finished gameplay.".format(MAP_PATH))


main()
