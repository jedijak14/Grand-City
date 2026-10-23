"""Download a pinned set of Kenney CC0 GLBs from the GitHub community mirror.

Uses only GitHub's API. The downloaded files retain kit folders and the shared kit textures
needed by GLBs. The GitHub mirror is not the author; source/licensing notes are next to assets.
"""
from __future__ import annotations
import base64
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / "ModelLibrary/Community/Kenney"
REPO = "shorepine/kenney"
REVISION = "3694c6879e487c108f55677be7dd2ca75b07cc3b"
KITS = {
    "city-commercial": [f"building-{x}.glb" for x in "abcdefghijklmn"]
        + [f"building-skyscraper-{x}.glb" for x in "abcde"],
    "city-suburban": [f"building-type-{x}.glb" for x in "abcdefghijklmnopqrst"]
        + ["fence.glb", "planter.glb", "tree-large.glb", "tree-small.glb", "driveway-long.glb"],
    "city-industrial": [f"building-{x}.glb" for x in "abcdefghij"]
        + ["detail-tank.glb", "chimney-large.glb"],
    "city-roads": [
        "road-straight.glb", "road-crossroad.glb", "road-crossing.glb",
        "road-bend.glb", "road-curve.glb", "road-bridge.glb",
        "light-square.glb", "road-end-round.glb", "road-driveway-single.glb",
        "road-driveway-double.glb", "road-bend-sidewalk.glb",
    ],
    "car": ["van.glb", "sedan.glb", "suv.glb", "taxi.glb", "ambulance.glb"],
    "nature": ["sign.glb"],
    "watercraft": ["boat-speed-a.glb", "boat-fishing-small.glb", "boat-sail-a.glb"],
}

def gh_content(path: str) -> bytes:
    result = subprocess.run(
        ["gh", "api", f"repos/{REPO}/contents/{path}?ref={REVISION}"],
        check=True, capture_output=True, text=True,
    )
    payload = json.loads(result.stdout)
    if payload.get("encoding") != "base64" or not payload.get("content"):
        raise RuntimeError(f"GitHub API did not return base64 content for {path}")
    return base64.b64decode(payload["content"])

def main() -> None:
    DEST.mkdir(parents=True, exist_ok=True)
    for path in ("LICENSE.txt", "3d/kits.tsv"):
        out = DEST / ("KENNEY_LICENSE_CC0.txt" if path == "LICENSE.txt" else "KIT_INDEX.tsv")
        out.write_bytes(gh_content(path))
    for kit, files in KITS.items():
        folder = DEST / kit
        folder.mkdir(parents=True, exist_ok=True)
        for filename in files:
            path = f"3d/{kit}/{filename}"
            out = folder / filename
            if not out.exists():
                out.write_bytes(gh_content(path))
        texture = f"3d/{kit}/Textures/colormap.png"
        try:
            (folder / "Textures").mkdir(exist_ok=True)
            out = folder / "Textures/colormap.png"
            if not out.exists(): out.write_bytes(gh_content(texture))
        except subprocess.CalledProcessError:
            pass  # Some kits embed their colors and have no external colormap.
        print(f"{kit}: {len(files)} models")
    print(f"Pinned commit: {REVISION}")

if __name__ == "__main__":
    main()
