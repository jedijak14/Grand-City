"""Generate an original, detailed vehicle prototype GLB for Grand City.

Requires: python -m pip install -r Tools/requirements-models.txt
Run from the repository root. Output dimensions are Unreal centimeters; +X is forward,
+Y is passenger side, +Z is up. This is an unrigged static visual asset, not a finished
production car or an in-game screenshot. Update the design and license notes before shipping.
"""
from __future__ import annotations

import math
from pathlib import Path

import numpy as np
import trimesh
from trimesh.visual.material import PBRMaterial
from trimesh.visual.texture import TextureVisuals

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "ModelLibrary" / "PublicGLB" / "grandcity_arden_gt.glb"

# Unreal units (cm), with a restrained original teal metallic / dark glass palette.
MATERIALS = [
    PBRMaterial(name="Arden deep tidal teal", baseColorFactor=[19, 78, 82, 255], metallicFactor=0.82, roughnessFactor=0.24),
    PBRMaterial(name="Satin graphite", baseColorFactor=[24, 29, 34, 255], metallicFactor=0.38, roughnessFactor=0.42),
    PBRMaterial(name="Smoked blue glass", baseColorFactor=[16, 32, 40, 255], metallicFactor=0.22, roughnessFactor=0.16),
    PBRMaterial(name="Brushed alloy", baseColorFactor=[160, 174, 179, 255], metallicFactor=0.92, roughnessFactor=0.23),
    PBRMaterial(name="Warm ivory lamp", baseColorFactor=[255, 244, 208, 255], metallicFactor=0.12, roughnessFactor=0.18, emissiveFactor=[0.35, 0.30, 0.18]),
    PBRMaterial(name="Signal red lens", baseColorFactor=[170, 22, 27, 255], metallicFactor=0.20, roughnessFactor=0.19, emissiveFactor=[0.18, 0.015, 0.01]),
    PBRMaterial(name="Tire rubber", baseColorFactor=[20, 22, 23, 255], metallicFactor=0.0, roughnessFactor=0.84),
    PBRMaterial(name="Cabin leather", baseColorFactor=[42, 35, 30, 255], metallicFactor=0.0, roughnessFactor=0.72),
    PBRMaterial(name="Amber marker", baseColorFactor=[232, 112, 25, 255], metallicFactor=0.12, roughnessFactor=0.2),
]

PARTS: list[tuple[trimesh.Trimesh, int]] = []
UNSCALED_PARTS: set[int] = set()


def add(mesh: trimesh.Trimesh, material_index: int) -> None:
    if mesh is None or len(mesh.faces) == 0:
        return
    mesh.remove_unreferenced_vertices()
    PARTS.append((mesh, material_index))


def box(extents, center, material, rotation=None):
    mesh = trimesh.creation.box(extents=np.asarray(extents, dtype=float))
    if rotation is not None:
        mesh.apply_transform(rotation)
    mesh.apply_translation(center)
    add(mesh, material)
    return mesh


def ellipsoid(center, radii, material, subdivisions=3):
    mesh = trimesh.creation.icosphere(subdivisions=subdivisions, radius=1.0)
    mesh.apply_scale(radii)
    mesh.apply_translation(center)
    add(mesh, material)
    return mesh


def cylinder_between(start, end, radius, material, sections=20):
    start = np.asarray(start, dtype=float)
    end = np.asarray(end, dtype=float)
    vector = end - start
    length = float(np.linalg.norm(vector))
    if length < 1e-5:
        return
    mesh = trimesh.creation.cylinder(radius=radius, height=length, sections=sections)
    rotation = trimesh.geometry.align_vectors([0.0, 0.0, 1.0], vector / length)
    if rotation is not None:
        mesh.apply_transform(rotation)
    mesh.apply_translation((start + end) * 0.5)
    add(mesh, material)


def rounded_ring(x, width, bottom, top, count=72, exponent=3.6):
    """Rounded-rectangle Y/Z profile with softened corners."""
    angles = np.arange(count) * (2.0 * math.pi / count)
    c, s = np.cos(angles), np.sin(angles)
    power = 2.0 / exponent
    y = width * np.sign(c) * np.abs(c) ** power
    half_height = (top - bottom) * 0.5
    mid_height = (top + bottom) * 0.5
    z = mid_height + half_height * np.sign(s) * np.abs(s) ** power
    return np.column_stack((np.full(count, x), y, z))


def loft(stations, material, ring_count=72, exponent=3.6):
    rings = [rounded_ring(*station, count=ring_count, exponent=exponent) for station in stations]
    vertices = np.vstack(rings)
    faces = []
    for ring in range(len(rings) - 1):
        a0, b0 = ring * ring_count, (ring + 1) * ring_count
        for i in range(ring_count):
            j = (i + 1) % ring_count
            faces.append([a0 + i, a0 + j, b0 + i])
            faces.append([a0 + j, b0 + j, b0 + i])
    # Close the front and rear ends with center fans, retaining stable index winding.
    for ring_id in (0, len(rings) - 1):
        ring_start = ring_id * ring_count
        center = vertices[ring_start:ring_start + ring_count].mean(axis=0)
        center_index = len(vertices)
        vertices = np.vstack((vertices, center))
        for i in range(ring_count):
            j = (i + 1) % ring_count
            faces.append([center_index, ring_start + i, ring_start + j])
    mesh = trimesh.Trimesh(vertices=vertices, faces=np.asarray(faces), process=True)
    add(mesh, material)
    return mesh


def curved_panel(points, material, rows=16, columns=32, offset=0.0):
    """Grid a gently curved four-corner panel in XYZ; panels are separate layered surfaces."""
    p00, p10, p11, p01 = [np.asarray(p, dtype=float) for p in points]
    vertices = []
    faces = []
    for row in range(rows + 1):
        v = row / rows
        left = p00 * (1.0 - v) + p01 * v
        right = p10 * (1.0 - v) + p11 * v
        for col in range(columns + 1):
            u = col / columns
            p = left * (1.0 - u) + right * u
            p[1] += math.copysign(offset, p[1]) if p[1] else 0.0
            vertices.append(p)
    for row in range(rows):
        for col in range(columns):
            a = row * (columns + 1) + col
            b = a + 1
            c = a + columns + 1
            d = c + 1
            faces.extend(([a, c, b], [b, c, d]))
    mesh = trimesh.Trimesh(vertices=np.asarray(vertices), faces=np.asarray(faces), process=True)
    add(mesh, material)


def torus_y(center, major_radius, minor_radius, material, ring_segments=112, tube_segments=28):
    """Torus with its axle on the Y axis, for detailed tires and wheel trim."""
    cx, cy, cz = center
    vertices, faces = [], []
    for i in range(ring_segments):
        theta = 2.0 * math.pi * i / ring_segments
        for j in range(tube_segments):
            phi = 2.0 * math.pi * j / tube_segments
            radial = major_radius + minor_radius * math.cos(phi)
            vertices.append((cx + radial * math.cos(theta), cy + minor_radius * math.sin(phi), cz + radial * math.sin(theta)))
    for i in range(ring_segments):
        for j in range(tube_segments):
            a = i * tube_segments + j
            b = ((i + 1) % ring_segments) * tube_segments + j
            c = i * tube_segments + (j + 1) % tube_segments
            d = ((i + 1) % ring_segments) * tube_segments + (j + 1) % tube_segments
            faces.extend(([a, b, c], [c, b, d]))
    add(trimesh.Trimesh(vertices=np.asarray(vertices), faces=np.asarray(faces), process=True), material)


def wheel(x, side, z=43.0):
    part_start = len(PARTS)
    y = side * 85.0
    # Dense rounded tire carcass and subtle tread shoulders.
    torus_y((x, y, z), 28.0, 12.0, 6, ring_segments=128, tube_segments=36)
    torus_y((x, y + side * 9.0, z), 27.4, 1.6, 6, ring_segments=112, tube_segments=12)
    torus_y((x, y - side * 9.0, z), 27.4, 1.6, 6, ring_segments=112, tube_segments=12)

    # Bright forged multi-spoke alloy face and inner brake hardware.
    disc = trimesh.creation.cylinder(radius=23.0, height=3.6, sections=96)
    rot = trimesh.transformations.rotation_matrix(math.pi / 2.0, [1, 0, 0])
    disc.apply_transform(rot)
    disc.apply_translation((x, y + side * 9.2, z))
    add(disc, 3)
    torus_y((x, y + side * 11.3, z), 21.0, 2.3, 3, ring_segments=96, tube_segments=16)
    torus_y((x, y + side * 11.5, z), 7.1, 1.8, 3, ring_segments=72, tube_segments=16)
    hub = trimesh.creation.cylinder(radius=5.7, height=5.0, sections=48)
    hub.apply_transform(rot)
    hub.apply_translation((x, y + side * 11.6, z))
    add(hub, 1)

    for spoke in range(10):
        theta = 2.0 * math.pi * spoke / 10.0 + 0.12
        inner = np.array([x + 6.5 * math.cos(theta), y + side * 11.2, z + 6.5 * math.sin(theta)])
        outer = np.array([x + 20.0 * math.cos(theta + 0.17), y + side * 11.2, z + 20.0 * math.sin(theta + 0.17)])
        cylinder_between(inner, outer, 1.65, 3, sections=12)
    for bolt in range(5):
        theta = 2.0 * math.pi * bolt / 5.0
        point = (x + 8.6 * math.cos(theta), y + side * 12.0, z + 8.6 * math.sin(theta))
        ellipsoid(point, (1.15, 0.8, 1.15), 3, subdivisions=2)

    # Fine circumferential shoulder ribs keep the tire silhouette smooth at a distance.
    for band_y in (-7.0, -3.5, 3.5, 7.0):
        torus_y((x, y + band_y, z), 39.3, 0.55, 6, ring_segments=112, tube_segments=8)

    UNSCALED_PARTS.update(range(part_start, len(PARTS)))


def build_vehicle():
    # Sculpted lower body; 84 stations x 96 rings create a smooth high-density silhouette.
    lower_profile = [
        (-229, 62, 30, 92), (-220, 76, 27, 108), (-202, 83, 24, 119),
        (-170, 86, 23, 127), (-125, 88, 22, 132), (-70, 90, 22, 130),
        (0, 91, 22, 129), (65, 91, 22, 130), (120, 87, 23, 132),
        (165, 83, 25, 128), (202, 78, 27, 120), (222, 69, 31, 107), (230, 50, 35, 85),
    ]
    dense_lower = []
    wheel_centres = (129.0, -137.0)
    wheel_arch_radius = 50.0
    wheel_top = 97.0
    for i in range(len(lower_profile) - 1):
        a, b = lower_profile[i], lower_profile[i + 1]
        for step in range(7):
            t = step / 7.0
            station = [(1.0 - t) * a[j] + t * b[j] for j in range(4)]
            x = station[0]
            arch_lift = 0.0
            for wheel_x in wheel_centres:
                normalized = abs(x - wheel_x) / wheel_arch_radius
                if normalized < 1.0:
                    arch_lift = max(arch_lift, (wheel_top - station[2]) * math.sqrt(1.0 - normalized * normalized))
            station[2] += arch_lift
            dense_lower.append(tuple(station))
    dense_lower.append(tuple(lower_profile[-1]))
    loft(dense_lower, 0, ring_count=96, exponent=4.8)

    # Fastback greenhouse and roof, integrated into the body-colored silhouette.
    cabin_profile = [
        (-148, 54, 127, 137), (-136, 64, 128, 151), (-120, 69, 129, 168),
        (-102, 69, 130, 184), (-82, 67, 130, 198), (-58, 64, 130, 207),
        (10, 64, 130, 207), (37, 65, 130, 205), (57, 68, 129, 194),
        (78, 70, 128, 176), (102, 70, 127, 155), (118, 62, 126, 137),
    ]
    loft(cabin_profile, 0, ring_count=80, exponent=3.0)

    # Curved smoked glass, separated from the painted metal. Panels are dense enough for smooth reflections.
    for side in (-1, 1):
        y_sign = side
        curved_panel(
            [(66, y_sign * 69, 145), (28, y_sign * 67, 193), (4, y_sign * 67, 194), (7, y_sign * 68, 146)],
            2, rows=20, columns=36, offset=2.2,
        )
        curved_panel(
            [(-5, y_sign * 68, 146), (-9, y_sign * 67, 194), (-65, y_sign * 66, 194), (-81, y_sign * 67, 146)],
            2, rows=20, columns=48, offset=2.2,
        )
        # Narrow polished pillars between glass panels.
        cylinder_between((5, y_sign * 69, 143), (1, y_sign * 68, 198), 2.6, 3, sections=16)
        cylinder_between((-78, y_sign * 69, 140), (-62, y_sign * 67, 198), 3.2, 0, sections=16)
        # Door shut lines and sculpted rocker strips.
        for x in (27.0, -84.0):
            cylinder_between((x, y_sign * 89, 48), (x - 5, y_sign * 88, 128), 0.8, 1, sections=10)
        cylinder_between((100, y_sign * 88, 42), (-140, y_sign * 89, 39), 2.0, 1, sections=20)
        # Flush handles and mirrors on short swept stems.
        ellipsoid((-38, y_sign * 91, 111), (12, 2.0, 2.4), 3, subdivisions=2)
        cylinder_between((75, y_sign * 79, 142), (90, y_sign * 92, 150), 3.0, 3, sections=20)
        ellipsoid((94, y_sign * 93, 151), (15, 8, 7), 0, subdivisions=3)
        ellipsoid((103, y_sign * 94, 152), (7.5, 6.8, 4.7), 2, subdivisions=3)

    # Panoramic windshield and sloped rear screen.
    curved_panel([(102, -65, 137), (102, 65, 137), (55, 62, 190), (55, -62, 190)], 2, rows=24, columns=48, offset=2.0)
    curved_panel([(-126, -60, 143), (-126, 60, 143), (-88, 62, 193), (-88, -62, 193)], 2, rows=24, columns=48, offset=2.0)

    # Undertray, front/rear lips, grille and precision trim.
    box((315, 122, 13), (-3, 0, 20), 1)
    box((15, 156, 12), (222, 0, 37), 1)
    box((22, 158, 10), (-220, 0, 43), 1)
    box((5, 52, 25), (230, 0, 69), 1)
    for slat in range(9):
        box((4.5, 48, 2.0), (233, 0, 51 + slat * 4.4), 3)

    # Lighting: paired swept LED headlamps, side markers and rear light bars.
    for side in (-1, 1):
        ellipsoid((226, side * 49, 104), (7.0, 25.0, 8.2), 1, subdivisions=3)
        ellipsoid((231, side * 49, 105), (3.8, 20.0, 4.0), 4, subdivisions=3)
        box((3.5, 18, 4.5), (221, side * 76, 84), 8)
        ellipsoid((-222, side * 56, 103), (5.5, 24.0, 8.0), 1, subdivisions=3)
        ellipsoid((-225, side * 56, 104), (3.0, 20.0, 5.4), 5, subdivisions=3)
        # Front and rear wheel assemblies; arches remain open for blueprint tuning.
        wheel(129, side)
        wheel(-137, side)
        # Raised, dark wheel-arch lips follow the tire profile instead of a solid slab.
        for wheel_x in (129.0, -137.0):
            arch_start = len(PARTS)
            for segment in range(36):
                a0 = math.pi * segment / 36.0
                a1 = math.pi * (segment + 1) / 36.0
                p0 = (wheel_x + 44.5 * math.cos(a0), side * 90.5, 43.0 + 44.5 * math.sin(a0))
                p1 = (wheel_x + 44.5 * math.cos(a1), side * 90.5, 43.0 + 44.5 * math.sin(a1))
                cylinder_between(p0, p1, 1.6, 1, sections=10)
            UNSCALED_PARTS.update(range(arch_start, len(PARTS)))
        # Polished roofline and side sill accents.
        cylinder_between((58, side * 71, 190), (-94, side * 69, 190), 1.0, 3, sections=12)

    # Subtle hood power lines, trunk crease, and rear diffuser details.
    for side in (-1, 1):
        cylinder_between((208, side * 43, 129), (112, side * 61, 134), 1.0, 3, sections=12)
        cylinder_between((-140, side * 55, 134), (-207, side * 55, 122), 1.0, 3, sections=12)
    for diffuser in range(7):
        box((16, 2.2, 11), (-217, -36 + diffuser * 12, 31), 3)

    # Cabin contents visible through dark glass: dashboard, seats, headrests and wheel.
    box((115, 114, 14), (18, 0, 122), 1)
    for seat_y in (-34, 34):
        for seat_x in (-67, 35):
            box((37, 30, 42), (seat_x, seat_y, 104), 7)
            box((18, 30, 34), (seat_x - 4, seat_y, 141), 7)
            box((20, 18, 13), (seat_x - 5, seat_y, 164), 7)
    box((48, 120, 9), (-30, 0, 131), 1)
    cylinder_between((70, -33, 141), (60, -33, 158), 1.3, 3, sections=12)
    # Steering wheel ring and center boss.
    torus_y((59, -33, 159), 11.0, 1.4, 3, ring_segments=48, tube_segments=12)
    ellipsoid((59, -33, 159), (3.5, 2.0, 3.5), 1, subdivisions=2)
    # Mirror stalk, exhaust finishers, tail badge and subtle roof antenna.
    cylinder_between((20, 0, 207), (-10, 0, 211), 1.2, 1, sections=12)
    for side in (-1, 1):
        cylinder_between((-225, side * 50, 34), (-239, side * 50, 34), 6.0, 3, sections=32)
        torus_y((-239, side * 50, 34), 4.0, 1.2, 1, ring_segments=40, tube_segments=12)
    ellipsoid((-230, 0, 94), (2, 9, 3.2), 3, subdivisions=2)


def export_glb():
    # trimesh's GLB exporter treats a mesh as one primitive/material. Merge components
    # by material, then export the nine material groups as separate nodes in one scene.
    # This preserves PBR assignments in standard GLB viewers and UE's glTF scene importer.
    scene = trimesh.Scene()
    total_vertices = 0
    total_faces = 0
    height_scale = 0.75
    wheel_axis_height = 43.0
    for material_index, material in enumerate(MATERIALS):
        group = []
        for part_index, (part_mesh, part_material) in enumerate(PARTS):
            if part_material != material_index:
                continue
            mesh = part_mesh.copy()
            if part_index not in UNSCALED_PARTS:
                mesh.vertices[:, 2] = wheel_axis_height + (mesh.vertices[:, 2] - wheel_axis_height) * height_scale
            group.append(mesh)
        if not group:
            continue
        combined = trimesh.util.concatenate(group)
        # Force smooth per-vertex normals into trimesh's export cache; without this,
        # GLB can omit NORMAL attributes and the metal body renders faceted in UE.
        _ = combined.vertex_normals
        combined.visual = TextureVisuals(material=material)
        combined.metadata["material_group"] = material.name
        scene.add_geometry(combined, geom_name=material.name, node_name=material.name)
        total_vertices += len(combined.vertices)
        total_faces += len(combined.faces)

    scene.metadata["asset_name"] = "Grand City Arden GT"
    scene.metadata["description"] = "Original detailed sedan concept mesh; Unreal centimeters, +X forward, +Z up."
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(scene.export(file_type="glb"))
    print("Wrote:", OUTPUT)
    print("Vertices:", total_vertices, "Triangles:", total_faces, "Material nodes:", len(scene.geometry))
    print("Bytes:", OUTPUT.stat().st_size)


if __name__ == "__main__":
    build_vehicle()
    export_glb()
