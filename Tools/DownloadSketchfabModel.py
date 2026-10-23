"""Download a Sketchfab model you are authorized to use into SketchfabDropIn.

Set SKETCHFAB_API_TOKEN in the local environment; never put the token in this file or Git.
Example:
  SKETCHFAB_API_TOKEN=... python Tools/DownloadSketchfabModel.py 935d92e43c4649a683777cc0d2bfe7fa

Sketchfab's download permission/license remains per-model. The script does not infer commercial
rights: review the model page and add attribution/license notes to the generated CREDITS.txt.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import tempfile
import urllib.error
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
API = "https://api.sketchfab.com/v3/models/{}/download"
DEST = ROOT / "ModelLibrary" / "SketchfabDropIn"


def request(url: str, token: str | None = None, timeout: int = 60):
    headers = {"User-Agent": "GridlockAssetImport/1.0"}
    if token:
        headers["Authorization"] = "Token " + token
    req = urllib.request.Request(url, headers=headers)
    return urllib.request.urlopen(req, timeout=timeout)


def safe_extract(archive: Path, destination: Path) -> list[Path]:
    destination = destination.resolve()
    written = []
    with zipfile.ZipFile(archive) as zf:
        for member in zf.infolist():
            target = (destination / member.filename).resolve()
            if target != destination and destination not in target.parents:
                raise RuntimeError("Unsafe path in Sketchfab archive: {}".format(member.filename))
        zf.extractall(destination)
        written = [destination / n for n in zf.namelist() if not n.endswith("/")]
    return written


def download(uid: str, slug: str) -> Path:
    token = os.environ.get("SKETCHFAB_API_TOKEN", "").strip()
    if not token:
        raise RuntimeError("Set SKETCHFAB_API_TOKEN in your local environment; no token is stored in the project.")
    api_url = API.format(uid)
    try:
        with request(api_url, token) as response:
            downloads = json.load(response)
    except urllib.error.HTTPError as error:
        detail = error.read(1024).decode("utf-8", errors="replace")
        if error.code in (401, 403):
            raise RuntimeError("Sketchfab rejected the token or download permission (HTTP {}). Check the token and model download/license settings.".format(error.code)) from error
        raise RuntimeError("Sketchfab download metadata failed (HTTP {}): {}".format(error.code, detail)) from error

    folder = DEST / "{}-{}".format(slug, uid[:8])
    folder.mkdir(parents=True, exist_ok=True)
    glb_info = downloads.get("glb")
    if glb_info and glb_info.get("url"):
        output = folder / (slug + ".glb")
        with request(glb_info["url"]) as response, output.open("wb") as target:
            shutil.copyfileobj(response, target)
        return output

    gltf_info = downloads.get("gltf")
    if not gltf_info or not gltf_info.get("url"):
        raise RuntimeError("Sketchfab returned no GLB or glTF download. Confirm this model is downloadable for this account.")
    with tempfile.TemporaryDirectory(prefix="gridlock-sketchfab-") as tmp:
        archive = Path(tmp) / "model.zip"
        with request(gltf_info["url"]) as response, archive.open("wb") as target:
            shutil.copyfileobj(response, target)
        files = safe_extract(archive, folder)
    gltfs = [p for p in files if p.suffix.lower() == ".gltf"]
    if not gltfs:
        raise RuntimeError("Downloaded archive did not contain a .gltf scene.")
    return gltfs[0]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("uid", help="Sketchfab model UID from its URL")
    parser.add_argument("--slug", help="Short filename label (defaults to model UID)")
    parser.add_argument("--source-url", help="Canonical Sketchfab model page URL for the credit record")
    args = parser.parse_args()
    if not re.fullmatch(r"[a-fA-F0-9]{32}", args.uid):
        parser.error("uid must be a 32-character Sketchfab model ID")
    slug = re.sub(r"[^A-Za-z0-9_-]+", "-", args.slug or args.uid[:8]).strip("-").lower()
    if not slug:
        slug = "model"
    path = download(args.uid.lower(), slug)
    page = args.source_url or "https://sketchfab.com/models/{}".format(args.uid.lower())
    credits = path.parent / "CREDITS.txt"
    credits.write_text(
        "Sketchfab source page: {}\nModel UID: {}\nAuthor: VERIFY AND FILL IN\n"
        "License: VERIFY ON THE MODEL PAGE (do not infer from another model or repository)\n"
        "Attribution text: VERIFY AND FILL IN\n\n"
        "Before production use: inspect geometry, materials, textures, trademarks, topology, "
        "LOD/collision needs, and commercial-use rights.\n".format(page, args.uid.lower()),
        encoding="utf-8")
    print("Downloaded source file: {}".format(path))
    print("Review and complete attribution/license record: {}".format(credits))
    print("No Unreal import/runtime validation was performed by this downloader.")


if __name__ == "__main__":
    main()
