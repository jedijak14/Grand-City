"""Audit selected starter-city source GLBs against a deliberately strict triangle-density gate.

This is a geometry-density preflight, not an art-quality certification: triangles cannot prove
realism, correct topology, texture quality, rigging, or UE import readiness. UE 5.8 visual and
performance review is still mandatory. Run with `python Tools/ValidateProductionAssetDensities.py`.
"""
from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# These are minimum triangle counts per asset (not per merged scene). They intentionally reject
# stylized low-poly packs from the production candidate list until an art-reviewed replacement is
# available. High counts alone do not make an asset production-ready.
REQUIREMENTS = {
    "architectural buildings": (100_000, [
        *(ROOT / "ModelLibrary/Community/Kenney/city-commercial" / ("building-{}.glb".format(letter))
          for letter in "abcdefghijklmn"),
        ROOT / "ModelLibrary/Community/Kenney/city-commercial/building-skyscraper-a.glb",
        ROOT / "ModelLibrary/Community/Kenney/city-commercial/building-skyscraper-b.glb",
        ROOT / "ModelLibrary/Community/Kenney/city-suburban/building-type-b.glb",
        ROOT / "ModelLibrary/Community/Kenney/city-industrial/building-a.glb",
    ]),
    "airport terminal": (250_000, [ROOT / "ModelLibrary/PublicGLB/gridlock_airport_terminal.glb"]),
    "vehicles": (50_000, [
        ROOT / "ModelLibrary/Community/Kenney/car/van.glb",
        ROOT / "ModelLibrary/Community/Kenney/car/sedan.glb",
    ]),
    "watercraft": (50_000, [ROOT / "ModelLibrary/Community/Kenney/watercraft/boat-speed-a.glb"]),
    "helicopter": (50_000, [ROOT / "ModelLibrary/Community/FlightAviation/helicopter.glb"]),
    "transit structural models": (50_000, [
        ROOT / "ModelLibrary/PublicGLB/gridlock_escalator_step.glb",
        ROOT / "ModelLibrary/PublicGLB/gridlock_escalator_railings.glb",
        ROOT / "ModelLibrary/PublicGLB/gridlock_elevator_guide_frame.glb",
    ]),
    "street infrastructure and props": (50_000, [
        ROOT / "ModelLibrary/Community/Kenney/city-roads/road-straight.glb",
        ROOT / "ModelLibrary/Community/Kenney/city-roads/road-crossroad.glb",
        ROOT / "ModelLibrary/Community/Kenney/city-roads/light-square.glb",
        ROOT / "ModelLibrary/Community/Kenney/nature/sign.glb",
    ]),
    "landscaping": (50_000, [
        ROOT / "ModelLibrary/Community/Kenney/city-suburban/tree-large.glb",
        ROOT / "ModelLibrary/Community/Kenney/city-suburban/tree-small.glb",
    ]),
    "surfboard": (50_000, [ROOT / "ModelLibrary/PublicGLB/gridlock_surfboard.glb"]),
}


def triangle_count(path: Path) -> int:
    """Read triangle primitive accessor counts directly from a GLB JSON chunk."""
    raw = path.read_bytes()
    if len(raw) < 20 or raw[:4] != b"glTF":
        raise ValueError("not a binary glTF (GLB)")
    version, declared_size = struct.unpack_from("<II", raw, 4)
    if version != 2 or declared_size != len(raw):
        raise ValueError("invalid GLB v2 header or byte length")
    offset = 12
    while offset + 8 <= len(raw):
        chunk_size, chunk_type = struct.unpack_from("<II", raw, offset)
        offset += 8
        chunk = raw[offset:offset + chunk_size]
        offset += chunk_size
        if chunk_type == 0x4E4F534A:  # JSON
            document = json.loads(chunk.decode("utf-8"))
            break
    else:
        raise ValueError("missing GLB JSON chunk")

    accessors = document.get("accessors", [])
    total = 0
    for mesh in document.get("meshes", []):
        for primitive in mesh.get("primitives", []):
            mode = primitive.get("mode", 4)
            if mode != 4:
                raise ValueError("non-triangle primitive mode {} is not accepted".format(mode))
            if "indices" in primitive:
                index_count = accessors[primitive["indices"]]["count"]
            else:
                position = primitive.get("attributes", {}).get("POSITION")
                if position is None:
                    raise ValueError("primitive has no POSITION accessor")
                index_count = accessors[position]["count"]
            if index_count % 3:
                raise ValueError("triangle index count is not divisible by three")
            total += index_count // 3
    if total <= 0:
        raise ValueError("GLB has no triangle geometry")
    return total


def main() -> int:
    failures = []
    print("Gridlock production geometry-density gate (counts are necessary, not sufficient)")
    for category, (minimum, assets) in REQUIREMENTS.items():
        print("\n{} — minimum {:,} triangles per GLB".format(category, minimum))
        for path in assets:
            if not path.is_file():
                failures.append((path, "missing", minimum))
                print("  FAIL  MISSING  {}".format(path.relative_to(ROOT)))
                continue
            try:
                count = triangle_count(path)
            except Exception as error:
                failures.append((path, str(error), minimum))
                print("  FAIL  INVALID  {}: {}".format(path.relative_to(ROOT), error))
                continue
            passed = count >= minimum
            print("  {}  {:>9,}  {}".format("PASS" if passed else "FAIL", count, path.relative_to(ROOT)))
            if not passed:
                failures.append((path, count, minimum))
    checked_paths = {
        path.resolve()
        for _, assets in REQUIREMENTS.values()
        for path in assets
    }
    unclassified = [path for path in sorted((ROOT / "ModelLibrary").rglob("*.glb"))
                    if path.resolve() not in checked_paths]
    if unclassified:
        print("\nOther source GLBs — minimum 50,000 triangles per model")
        for path in unclassified:
            try:
                count = triangle_count(path)
            except Exception as error:
                failures.append((path, str(error), 50_000))
                print("  FAIL  INVALID  {}: {}".format(path.relative_to(ROOT), error))
                continue
            passed = count >= 50_000
            print("  {}  {:>9,}  {}".format("PASS" if passed else "FAIL", count, path.relative_to(ROOT)))
            if not passed:
                failures.append((path, count, 50_000))

    total_checked = sum(len(assets) for _, assets in REQUIREMENTS.values()) + len(unclassified)
    print("\n{} source GLBs checked; {} fail the density gate.".format(total_checked, len(failures)))
    if failures:
        print("Do not label the starter city production-ready or substitute low-density assets as final art.")
        return 1
    print("Density gate passed. Artist review, topology/material checks, Unreal import and runtime validation remain required.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
