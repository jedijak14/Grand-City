"""Apply a simple fictional Coastline Airways vertex-color livery to the CC-BY A320 GLB."""
import json
import struct
import zlib
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[1] / "ModelLibrary/PublicGLB/gridlock_airliner_a320_base.glb"
DEST = Path(__file__).resolve().parents[1] / "ModelLibrary/PublicGLB/gridlock_coastline_a320.glb"


def unpack_glb(path):
    blob = path.read_bytes()
    if blob[:4] != b"glTF":
        raise ValueError("Not a GLB file")
    json_length, json_type = struct.unpack_from("<II", blob, 12)
    if json_type != 0x4E4F534A:
        raise ValueError("Missing GLB JSON chunk")
    document = json.loads(blob[20:20 + json_length])
    offset = 20 + json_length
    while offset % 4:
        offset += 1
    bin_length, bin_type = struct.unpack_from("<II", blob, offset)
    if bin_type != 0x004E4942:
        raise ValueError("Missing GLB binary chunk")
    return document, bytearray(blob[offset + 8:offset + 8 + bin_length])


def pack_glb(document, binary, output):
    json_bytes = json.dumps(document, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    json_bytes += b" " * ((-len(json_bytes)) % 4)
    binary += b"\0" * ((-len(binary)) % 4)
    total = 12 + 8 + len(json_bytes) + 8 + len(binary)
    with output.open("wb") as stream:
        stream.write(struct.pack("<4sII", b"glTF", 2, total))
        stream.write(struct.pack("<II", len(json_bytes), 0x4E4F534A))
        stream.write(json_bytes)
        stream.write(struct.pack("<II", len(binary), 0x004E4942))
        stream.write(binary)


def read_positions(document, binary, accessor_index):
    accessor = document["accessors"][accessor_index]
    view = document["bufferViews"][accessor["bufferView"]]
    start = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    stride = view.get("byteStride", 12)
    positions = []
    for index in range(accessor["count"]):
        positions.append(struct.unpack_from("<3f", binary, start + index * stride))
    return positions


def make_colors(positions):
    # Axes of the supplied model: X is span, Y runs nose-to-tail, Z is vertical.
    # White upper fuselage, deep-navy belly/tail, sea-teal waist stripe and small gold tail band.
    result = bytearray()
    for x, y, z in positions:
        color = (242, 244, 241, 255)
        if z < -125:
            color = (24, 43, 68, 255)
        if abs(x) < 82 and -92 < z < -38 and -590 < y < 535:
            color = (14, 139, 145, 255)
        if y < -500 and abs(x) < 128 and z > -150:
            color = (15, 43, 75, 255)
        if y < -645 and abs(x) < 105 and -45 < z < 70:
            color = (225, 173, 76, 255)
        if abs(x) > 505 and -300 < y < 280 and z > -115:
            color = (14, 139, 145, 255)
        result.extend(color)
    return result


def add_tail_logo(document, binary):
    # Simple gold Coastline "C" mark on both sides of the vertical tail.
    positions, normals, colors, indices = [], [], [], []
    bars = [(-694.0, -682.0, 8.0, 92.0), (-694.0, -610.0, 80.0, 92.0),
            (-694.0, -610.0, 8.0, 20.0), (-622.0, -610.0, 20.0, 80.0)]
    for side in (-1.0, 1.0):
        x = side * 43.0
        normal = (side, 0.0, 0.0)
        for y0, y1, z0, z1 in bars:
            base = len(positions)
            positions.extend([(x, y0, z0), (x, y1, z0), (x, y1, z1), (x, y0, z1)])
            normals.extend([normal] * 4)
            colors.extend([(232, 179, 79, 255)] * 4)
            indices.extend([base, base + 1, base + 2, base, base + 2, base + 3])

    def append_view(payload, target):
        while len(binary) % 4:
            binary.append(0)
        offset = len(binary)
        binary.extend(payload)
        view = {"buffer": 0, "byteOffset": offset, "byteLength": len(payload)}
        if target is not None:
            view["target"] = target
        document["bufferViews"].append(view)
        return len(document["bufferViews"]) - 1

    position_data = b"".join(struct.pack("<3f", *v) for v in positions)
    normal_data = b"".join(struct.pack("<3f", *v) for v in normals)
    index_data = b"".join(struct.pack("<H", i) for i in indices)
    pos_view = append_view(position_data, 34962)
    norm_view = append_view(normal_data, 34962)
    color_view = append_view(bytes(c for color in colors for c in color), 34962)
    index_view = append_view(index_data, 34963)
    pos_values = list(zip(*positions))
    document["accessors"].extend([
        {"bufferView": pos_view, "componentType": 5126, "count": len(positions), "type": "VEC3",
         "min": [min(axis) for axis in pos_values], "max": [max(axis) for axis in pos_values]},
        {"bufferView": norm_view, "componentType": 5126, "count": len(normals), "type": "VEC3"},
        {"bufferView": color_view, "componentType": 5121, "normalized": True, "count": len(colors), "type": "VEC4"},
        {"bufferView": index_view, "componentType": 5123, "count": len(indices), "type": "SCALAR"},
    ])
    pos_accessor, norm_accessor, color_accessor, index_accessor = range(len(document["accessors"]) - 4, len(document["accessors"]))
    document["meshes"].append({"name": "Coastline_Tail_Logo", "primitives": [{
        "attributes": {"POSITION": pos_accessor, "NORMAL": norm_accessor, "COLOR_0": color_accessor},
        "indices": index_accessor, "material": 0
    }]})
    logo_mesh_index = len(document["meshes"]) - 1
    document["nodes"].append({"name": "Coastline_Tail_Logo", "mesh": logo_mesh_index})
    scene = document["scenes"][document.get("scene", 0)]
    scene.setdefault("nodes", []).append(len(document["nodes"]) - 1)


def add_wing_fasteners(document, binary, source_positions):
    """Add small metallic flush fastener heads along the visible upper-wing panels."""
    from math import cos, sin, tau
    positions, normals, colors, indices = [], [], [], []
    for side in (-1.0, 1.0):
        for span_x in range(270, 471, 25):
            for panel_y in (-142.0, -119.0):
                x = side * float(span_x)
                nearby = [(px, py, pz) for px, py, pz in source_positions
                          if abs(px - x) < 24.0 and abs(py - panel_y) < 24.0]
                if not nearby:
                    continue
                # Match the sampled upper skin and offset slightly to avoid z-fighting.
                z = max(pz for _, _, pz in nearby) + 1.6
                radius = 2.5
                base = len(positions)
                positions.append((x, panel_y, z))
                normals.append((0.0, 0.0, 1.0))
                colors.append((120, 133, 143, 255))
                for step in range(8):
                    angle = tau * step / 8
                    positions.append((x + radius * cos(angle), panel_y + radius * sin(angle), z))
                    normals.append((0.0, 0.0, 1.0))
                    colors.append((171, 181, 184, 255))
                for step in range(8):
                    indices.extend((base, base + 1 + step, base + 1 + ((step + 1) % 8)))

    if not positions:
        return

    def append_view(payload, target):
        while len(binary) % 4:
            binary.append(0)
        offset = len(binary)
        binary.extend(payload)
        view = {"buffer": 0, "byteOffset": offset, "byteLength": len(payload)}
        if target is not None:
            view["target"] = target
        document["bufferViews"].append(view)
        return len(document["bufferViews"]) - 1

    pos_view = append_view(b"".join(struct.pack("<3f", *v) for v in positions), 34962)
    norm_view = append_view(b"".join(struct.pack("<3f", *v) for v in normals), 34962)
    color_view = append_view(bytes(c for color in colors for c in color), 34962)
    index_view = append_view(b"".join(struct.pack("<H", i) for i in indices), 34963)
    axes = list(zip(*positions))
    document["accessors"].extend([
        {"bufferView": pos_view, "componentType": 5126, "count": len(positions), "type": "VEC3",
         "min": [min(axis) for axis in axes], "max": [max(axis) for axis in axes]},
        {"bufferView": norm_view, "componentType": 5126, "count": len(normals), "type": "VEC3"},
        {"bufferView": color_view, "componentType": 5121, "normalized": True, "count": len(colors), "type": "VEC4"},
        {"bufferView": index_view, "componentType": 5123, "count": len(indices), "type": "SCALAR"},
    ])
    pos, normal, color, index = range(len(document["accessors"]) - 4, len(document["accessors"]))
    document["meshes"].append({"name": "Coastline_Wing_Fastener_Heads", "primitives": [{
        "attributes": {"POSITION": pos, "NORMAL": normal, "COLOR_0": color},
        "indices": index, "material": 0
    }]})
    document["nodes"].append({"name": "Coastline_Wing_Fastener_Heads", "mesh": len(document["meshes"]) - 1})
    document["scenes"][document.get("scene", 0)].setdefault("nodes", []).append(len(document["nodes"]) - 1)


def main():
    document, binary = unpack_glb(SOURCE)
    mesh = document["meshes"][0]
    primitive = mesh["primitives"][0]
    positions = read_positions(document, binary, primitive["attributes"]["POSITION"])
    colors = make_colors(positions)
    offset = len(binary)
    binary.extend(colors)
    while len(binary) % 4:
        binary.append(0)
    document["bufferViews"].append({"buffer": 0, "byteOffset": offset, "byteLength": len(colors), "target": 34962})
    document["accessors"].append({"bufferView": len(document["bufferViews"]) - 1, "componentType": 5121,
                                  "normalized": True, "count": len(positions), "type": "VEC4"})
    primitive["attributes"]["COLOR_0"] = len(document["accessors"]) - 1
    document["asset"]["generator"] = "Gridlock Coastline Airways livery tool (derived from AMV Lab CC-BY-4.0 A320)"
    add_tail_logo(document, binary)
    add_wing_fasteners(document, binary, positions)
    document["buffers"][0]["byteLength"] = len(binary)
    pack_glb(document, binary, DEST)
    print(f"Wrote {DEST} ({DEST.stat().st_size:,} bytes; {len(positions):,} colored vertices)")


if __name__ == "__main__":
    main()
