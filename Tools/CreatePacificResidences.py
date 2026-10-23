"""Build an original, detailed coastal-city residential landmark for Gridlock.

The model uses centimetres, Z-up, and a generated PBR base-color atlas. This is original
parametric architecture, not a real-world building replica. It is a hero environment asset;
use LODs/culling and a simplified collision mesh for production gameplay. Generated GLB is
not an Unreal import/runtime verification.

Run: .venv/bin/python Tools/CreatePacificResidences.py
"""
from __future__ import annotations

import math
from pathlib import Path

import numpy as np
import trimesh
from PIL import Image
from trimesh.visual.material import PBRMaterial
from trimesh.visual.texture import TextureVisuals

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "ModelLibrary/PublicGLB/gridlock_pacific_residences.glb"
ATLAS = ROOT / "ModelLibrary/Textures/gridlock_city_material_atlas.png"

# Atlas is arranged in four columns by three rows, sampled inside each tile to avoid seams.
# Indices: pale stone, cast concrete, anodized metal, reflective glass; wood, brushed steel,
# limestone, dark stone; teal tile, asphalt, sidewalk concrete, lush planting.
TILES = {"stone": 0, "concrete": 1, "metal": 2, "glass": 3, "wood": 4,
         "steel": 5, "limestone": 6, "darkstone": 7, "tile": 8,
         "asphalt": 9, "sidewalk": 10, "foliage": 11}

MAT_PROPS = {
    "stone": (0.06, 0.72), "concrete": (0.04, 0.88), "metal": (0.76, 0.29),
    "glass": (0.22, 0.15), "wood": (0.03, 0.65), "steel": (0.84, 0.25),
    "limestone": (0.02, 0.7), "darkstone": (0.03, 0.79), "tile": (0.12, 0.36),
    "asphalt": (0.0, 0.93), "sidewalk": (0.01, 0.84), "foliage": (0.0, 0.86),
}

# Store box/cylinder geometry as batched arrays per atlas tile to keep the GLB hierarchy compact.
BATCH = {key: [[], [], [], []] for key in TILES}


def _uv_for_tile(tile: int) -> np.ndarray:
    col, row = tile % 4, tile // 4
    # inset UVs by 3.5% of a tile to avoid texture bleeding at mip levels.
    inset = 0.035
    u0, u1 = col / 4 + inset / 4, (col + 1) / 4 - inset / 4
    # image rows start at top; glTF UV v=1 is the image top.
    v1, v0 = 1 - row / 3 - inset / 3, 1 - (row + 1) / 3 + inset / 3
    return np.asarray([[u0, v0], [u1, v0], [u1, v1], [u0, v1]], dtype=np.float32)


def _store(name: str, vertices: np.ndarray, faces: np.ndarray, tile_name: str,
           uv: np.ndarray | None = None) -> None:
    b = BATCH[tile_name]
    vertices = vertices.astype(np.float32)
    b[0].append(vertices)
    b[1].append(faces.astype(np.uint32) + sum(len(v) for v in b[0][:-1]))
    b[2].append(np.full(len(faces), TILES[tile_name], dtype=np.uint8))
    if uv is None:
        uv = np.repeat(_uv_for_tile(TILES[tile_name]).mean(axis=0)[None, :], len(vertices), axis=0)
    b[3].append(np.asarray(uv, dtype=np.float32))


def box(center, size, tile_name: str) -> None:
    """Append a UV-mapped cuboid with independent face charts."""
    cx, cy, cz = map(float, center)
    sx, sy, sz = (float(x) * 0.5 for x in size)
    corners = np.asarray([
        [cx-sx, cy-sy, cz-sz], [cx+sx, cy-sy, cz-sz],
        [cx+sx, cy+sy, cz-sz], [cx-sx, cy+sy, cz-sz],
        [cx-sx, cy-sy, cz+sz], [cx+sx, cy-sy, cz+sz],
        [cx+sx, cy+sy, cz+sz], [cx-sx, cy+sy, cz+sz],
    ], dtype=np.float32)
    quads = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
             (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    vertices, faces, uvs = [], [], []
    base_uv = _uv_for_tile(TILES[tile_name])
    for q in quads:
        start = len(vertices)
        vertices.extend(corners[list(q)])
        uvs.extend(base_uv)
        faces.extend(([start, start+1, start+2], [start, start+2, start+3]))
    _store(tile_name, np.asarray(vertices), np.asarray(faces), tile_name, np.asarray(uvs))


def cylinder(a, b, radius: float, tile_name: str, segments: int = 12) -> None:
    """Create closed, round architectural detail between arbitrary endpoints."""
    a, b = np.asarray(a, dtype=np.float64), np.asarray(b, dtype=np.float64)
    axis = b - a
    length = float(np.linalg.norm(axis))
    if length <= 1e-5:
        return
    axis /= length
    reference = np.array([0.0, 0.0, 1.0]) if abs(axis[2]) < 0.92 else np.array([0.0, 1.0, 0.0])
    u = np.cross(axis, reference); u /= np.linalg.norm(u)
    v = np.cross(axis, u); v /= np.linalg.norm(v)
    theta = np.arange(segments) * (2 * math.pi / segments)
    ring = (np.cos(theta)[:, None] * u + np.sin(theta)[:, None] * v) * radius
    vertices = np.vstack((a + ring, b + ring, a, b)).astype(np.float32)
    faces = []
    for i in range(segments):
        j = (i + 1) % segments
        faces.extend(([i, j, segments+i], [j, segments+j, segments+i]))
        faces.append([2*segments, j, i])
        faces.append([2*segments+1, segments+i, segments+j])
    _store(tile_name, vertices, np.asarray(faces), tile_name)


def balcony(x0: float, y: float, z: float, width: float, depth: float,
            front_sign: int, glass: bool) -> None:
    """Balcony slab, layered fascia, glass guard, handrail, and repeated stanchions."""
    box((x0, y, z), (width, depth, 15), "concrete")
    box((x0, y + front_sign * (depth * 0.5 - 3), z - 5), (width + 8, 8, 8), "metal")
    rail_z = z + 61
    front = y + front_sign * (depth - 12)
    side_y0, side_y1 = sorted((y - depth * 0.5 + 12, y + depth * 0.5 - 12))
    # Clear laminated glass panel is inset between dark alloy posts.
    if glass:
        box((x0, front, z + 31), (width - 26, 8, 54), "glass")
    cylinder((x0-width/2+8, front, rail_z), (x0+width/2-8, front, rail_z), 5, "steel", 16)
    for yy in (side_y0, side_y1):
        if glass:
            box((x0-width/2+8, yy, z+31), (8, depth-30, 54), "glass")
        cylinder((x0-width/2+8, yy, rail_z), (x0+width/2-8, yy, rail_z), 4, "steel", 12)
    count = max(5, int(width / 36))
    for i in range(count + 1):
        x = x0 - width/2 + 10 + (width-20)*i/count
        cylinder((x, front, z+5), (x, front, rail_z), 2.4, "steel", 10)
    # Layered planter at alternating bays gives the facade a planted, lived-in rhythm.
    if int((x0 + 700) // 260) % 2 == 0:
        planter_y = y - front_sign * (depth * 0.31)
        box((x0, planter_y, z+18), (width*0.55, 35, 24), "darkstone")
        for i in range(8):
            px = x0 - width*0.25 + i*width*0.5/7
            height = 20 + (i*17 % 24)
            cylinder((px, planter_y, z+30), (px+((i%3)-1)*7, planter_y, z+30+height),
                     3.5, "foliage", 8)
            for side in (-1, 1):
                box((px+side*8, planter_y, z+38+height*0.35), (13, 8, 8), "foliage")


def window_bay(x: float, y: float, z: float, width: float, height: float,
              depth: float, side: int, mullion: float = 8) -> None:
    """Inset glass and articulated anodized frame; side is -1/+1 wall normal."""
    box((x, y, z), (width, depth, height), "glass")
    # perimeter mullions, two verticals and an intermediate transom.
    for xx in (x-width/2, x+width/2):
        box((xx, y-side*depth*0.62, z), (mullion, depth*1.4, height+mullion), "metal")
    for zz in (z-height/2, z+height/2):
        box((x, y-side*depth*0.62, zz), (width+mullion, depth*1.4, mullion), "metal")
    box((x, y-side*depth*0.68, z+height*0.12), (width, depth*1.45, 5), "steel")


def main() -> None:
    if not ATLAS.exists():
        raise FileNotFoundError("Required generated atlas missing: {}".format(ATLAS))
    # Dimensions: 33 m x 22 m tower; 16 residential floors over a two-storey podium.
    tower_width, tower_depth = 3300.0, 2200.0
    podium_top, floor_h, floors = 730.0, 335.0, 16
    tower_top = podium_top + floor_h * floors

    # Ground podium and structural stone piers, with a recessed, glazed retail base.
    box((0, 0, 110), (3900, 2800, 220), "darkstone")
    box((0, 0, 295), (3780, 2700, 150), "limestone")
    box((0, 0, 500), (3650, 2600, 280), "concrete")
    box((0, 0, 642), (3680, 2650, 16), "metal")
    # Podium shop glazing, repeated storefront frames and deep canopy.
    for side in (-1, 1):
        wall_y = side * 1338
        for bay in range(12):
            x = -1650 + bay * 300
            box((x, wall_y, 375), (252, 15, 215), "glass")
            for dx in (-130, 130):
                box((x+dx, wall_y-side*12, 375), (9, 34, 230), "metal")
            box((x, wall_y-side*12, 482), (270, 32, 10), "steel")
        box((0, wall_y-side*34, 525), (3650, 150, 26), "metal")
    for side in (-1, 1):
        wall_x = side * 1840
        for bay in range(9):
            y = -1050 + bay*260
            box((wall_x, y, 385), (15, 210, 220), "glass")
            box((wall_x-side*12, y, 385), (34, 9, 238), "metal")
    # The tower is hollow behind its curtain wall: a compact service core, real floor plates,
    # and corner piers leave the window bands readable instead of hiding them behind a solid box.
    box((0, 0, (podium_top+tower_top)/2), (620, 520, tower_top-podium_top), "concrete")
    for floor_index in range(floors + 1):
        slab_z = podium_top + floor_index*floor_h
        box((0, 0, slab_z), (tower_width-90, tower_depth-100, 18), "concrete")
    for sx in (-1, 1):
        for sy in (-1, 1):
            box((sx*(tower_width/2-110), sy*(tower_depth/2-110), (podium_top+tower_top)/2),
                (90, 90, tower_top-podium_top), "limestone")
    columns = 12
    bay_w = tower_width / columns
    for floor in range(floors):
        z0 = podium_top + floor*floor_h
        zc = z0 + floor_h/2
        # Continuous recessed floor bands give each level a crisp architectural datum.
        for side in (-1, 1):
            y = side*(tower_depth/2 + 58)
            box((0, y, z0+8), (tower_width+80, 52, 24), "steel")
            box((0, y, z0+20), (tower_width+100, 14, 5), "metal")
            # Window panels are separated by limestone piers and dark vertical shadow joints.
            for bay in range(columns):
                x = -tower_width/2 + bay_w*(bay+0.5)
                window_bay(x, y-side*19, zc+8, bay_w-30, floor_h-86, 22, side)
                if bay % 3 == 0:
                    box((x, y-side*43, zc+8), (7, 15, floor_h-100), "steel")
                # Alternating sunshades provide fine horizontal facade depth.
                if (bay + floor) % 3 == 0:
                    box((x, y-side*48, zc+floor_h*0.31), (bay_w-18, 58, 9), "metal")
            # Corner pilasters with layered stone caps.
            for x in (-tower_width/2-10, tower_width/2+10):
                box((x, y-side*15, zc), (38, 44, floor_h-12), "limestone")
        # Side elevations: slightly narrower windows, deep sill and vertical fins.
        for side in (-1, 1):
            x = side*(tower_width/2+16)
            side_bays = 8
            side_w = tower_depth/side_bays
            for bay in range(side_bays):
                y = -tower_depth/2 + (bay+0.5)*side_w
                box((x, y, zc+8), (24, side_w-38, floor_h-92), "glass")
                for yy in (y-side_w/2+12, y+side_w/2-12):
                    box((x+side*18, yy, zc+8), (58, 9, floor_h-82), "metal")
                box((x+side*20, y, z0+floor_h-34), (56, side_w-35, 14), "steel")
        # Balconies on alternating front/back bays: deep, habitable and fully railed.
        if floor < floors-1:
            for side in (-1, 1):
                face_y = side*(tower_depth/2+28)
                for bay in range(columns):
                    if (bay + floor) % 2 != 0:
                        continue
                    x = -tower_width/2 + bay_w*(bay+0.5)
                    balcony(x, face_y, z0+68, bay_w*0.91, 142, side, glass=True)
                    # A slim vertical privacy screen at every third balcony.
                    if bay % 3 == 0:
                        for fin in range(5):
                            xx = x-bay_w*0.43+fin*11
                            box((xx, face_y-side*52, z0+90), (4, 12, 160), "wood")
        # Detailed sill upstands, drain scuppers and fascia seams on every second floor.
        if floor % 2 == 0:
            for side in (-1, 1):
                y = side*(tower_depth/2+35)
                for bay in range(columns):
                    x = -tower_width/2 + bay_w*(bay+0.5)
                    box((x, y, z0+floor_h-20), (bay_w-8, 18, 5), "darkstone")

    # Signature crown: stacked parapet, screened mechanical penthouse, and a thin pergola.
    box((0, 0, tower_top+42), (tower_width+110, tower_depth+100, 84), "limestone")
    box((0, 0, tower_top+152), (tower_width*0.66, tower_depth*0.72, 140), "darkstone")
    for side in (-1, 1):
        y = side*(tower_depth*0.36+12)
        for i in range(25):
            x = -tower_width*0.33 + i*(tower_width*0.66/24)
            box((x, y, tower_top+155), (20, 18, 142), "metal")
    box((0, 0, tower_top+236), (tower_width*0.74, tower_depth*0.8, 18), "steel")
    for x in np.arange(-tower_width*0.36, tower_width*0.37, 125):
        cylinder((x, -tower_depth*0.40, tower_top+246), (x, tower_depth*0.40, tower_top+246), 4.5, "steel", 12)
    for y in (-tower_depth*0.4, tower_depth*0.4):
        cylinder((-tower_width*0.37, y, tower_top+246), (tower_width*0.37, y, tower_top+246), 4.5, "steel", 12)
    # Rooftop lounge trellis posts and planter parapet.
    for x in (-900, -300, 300, 900):
        cylinder((x, 0, tower_top+245), (x, 0, tower_top+455), 8, "steel", 16)
    box((0, 0, tower_top+460), (2300, 1450, 22), "wood")
    for x in np.arange(-1050, 1051, 80):
        box((x, 0, tower_top+475), (18, 1420, 18), "wood")
    for side in (-1, 1):
        y = side*1050
        box((0, y, tower_top+270), (2400, 90, 44), "darkstone")
        for i in range(25):
            x = -1100 + i*92
            cylinder((x, y, tower_top+293), (x, y, tower_top+360+(i*13 % 45)), 4, "foliage", 8)

    # Ground-level landscaped forecourt and address plinths.
    box((0, -1550, -20), (5200, 950, 40), "sidewalk")
    for x in (-1800, -900, 0, 900, 1800):
        box((x, -1570, 34), (320, 200, 74), "darkstone")
        for i in range(12):
            xx = x - 105 + i*19
            cylinder((xx, -1570, 72), (xx+((i%3)-1)*10, -1570, 124+(i*7%30)),
                     5.5, "foliage", 8)
    # Monument entry portal with branded but fictional Gridlock address plate.
    box((0, -1345, 150), (750, 80, 300), "darkstone")
    box((0, -1392, 210), (650, 18, 90), "tile")
    for x in (-360, 360):
        box((x, -1360, 235), (42, 110, 470), "steel")
    box((0, -1360, 470), (770, 110, 42), "steel")

    # Batch same-material parts into a compact, named hierarchy and embed the generated atlas.
    atlas_image = Image.open(ATLAS).convert("RGB")
    scene = trimesh.Scene()
    triangle_total = 0
    for name, tile in TILES.items():
        vertex_groups, face_groups, uv_groups = BATCH[name][0], BATCH[name][1], BATCH[name][3]
        if not vertex_groups:
            continue
        vertices = np.concatenate(vertex_groups, axis=0)
        faces = np.concatenate(face_groups, axis=0)
        uv = np.concatenate(uv_groups, axis=0)
        triangle_total += len(faces)
        mat_verts = vertices
        material = PBRMaterial(
            name="Gridlock {} | PBR Atlas".format(name.title()),
            baseColorFactor=[255, 255, 255, 255],
            baseColorTexture=atlas_image,
            metallicFactor=MAT_PROPS[name][0],
            roughnessFactor=MAT_PROPS[name][1],
            alphaMode="OPAQUE",
        )
        mesh = trimesh.Trimesh(vertices=mat_verts, faces=faces, process=False)
        mesh.visual = TextureVisuals(uv=uv, material=material)
        scene.add_geometry(mesh, geom_name="Pacific Residences - {}".format(name.title()),
                           node_name="Pacific Residences - {}".format(name.title()))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_bytes(trimesh.exchange.gltf.export_glb(scene))
    print("Wrote {}".format(OUT))
    print("{} triangles | {} materials | bounds {}".format(
        triangle_total, len(scene.geometry), scene.bounds.tolist()))
    print("Atlas: {}".format(ATLAS))


if __name__ == "__main__":
    main()
