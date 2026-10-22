"""Create a simple editor-only greybox test map for the Grand City C++ systems.

Run from Unreal Editor: Tools > Execute Python Script. This is a test layout, not
finished art or evidence of gameplay. Save the generated map after running it.
"""
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


def cube(mesh, location, scale, label):
    actor = spawn(unreal.StaticMeshActor, location)
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_actor_label(label, mark_dirty=True)
    return actor


def main():
    if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
        raise RuntimeError("Could not create {}. Create an empty level manually and rerun.".format(MAP_PATH))

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
            mesh_component.set_static_mesh(cube_mesh)
            mesh_component.set_relative_scale3d(unreal.Vector(8.0, 8.0, 7.0))
        locations.append(actor)
    locations[3].set_editor_property("flight_destination", locations[4])
    locations[4].set_editor_property("flight_destination", locations[3])

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
