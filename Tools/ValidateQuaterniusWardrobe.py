"""Offline structural checks for the bundled Quaternius wardrobe GLBs.

This is not an Unreal import, retarget, clothing simulation, or gameplay test.
"""
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "ModelLibrary/Clothing/Quaternius"


def read_glb(path):
    data = path.read_bytes()
    if len(data) < 20 or data[:4] != b"glTF":
        raise ValueError("{} is not a GLB v2 file".format(path.name))
    version, total = struct.unpack_from("<II", data, 4)
    if version != 2 or total != len(data):
        raise ValueError("{} has an invalid GLB header/length".format(path.name))
    json_length, json_type = struct.unpack_from("<II", data, 12)
    if json_type != 0x4E4F534A:
        raise ValueError("{} has no JSON chunk".format(path.name))
    document = json.loads(data[20:20 + json_length])
    if "EXT_meshopt_compression" in document.get("extensionsRequired", []):
        raise ValueError("{} still requires meshopt decompression".format(path.name))
    return document


def main():
    models = sorted(ROOT.glob("*.glb"))
    if not models:
        raise SystemExit("No wardrobe GLBs found in {}".format(ROOT))
    canonical_joints = None
    failed = []
    for path in models:
        document = read_glb(path)
        if not document.get("skins"):
            failed.append("{}: no skeletal skins".format(path.name))
            continue
        if not document.get("animations"):
            failed.append("{}: no embedded animations".format(path.name))
            continue
        skeleton = document["skins"][0]
        joint_names = tuple(document["nodes"][i].get("name", "") for i in skeleton["joints"])
        if canonical_joints is None:
            canonical_joints = joint_names
        elif joint_names != canonical_joints:
            failed.append("{}: joint list differs; do not assume shared-skeleton retargeting".format(path.name))
        print("{}: {} mesh(es), {} skin(s), {} joints, {} animation clip(s)".format(
            path.name, len(document.get("meshes", [])), len(document["skins"]),
            len(joint_names), len(document["animations"])))
    if failed:
        raise SystemExit("\n".join(failed))
    print("Structural checks passed. Unreal import/retarget/Chaos Cloth still require UE 5.8 validation.")


if __name__ == "__main__":
    main()
