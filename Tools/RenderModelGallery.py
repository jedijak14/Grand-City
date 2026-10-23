"""Create offline shaded preview screenshots for every GLB in ModelLibrary.

These are lightweight CPU previews for asset review, not UE screenshots or quality certification.
Install Tools/requirements-models.txt, including Pillow, then run:
    python Tools/RenderModelGallery.py
Outputs individual PNGs and contact sheets under Docs/ModelGallery/.
"""
from __future__ import annotations

import html
import re
from pathlib import Path

import numpy as np
import trimesh
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
LIBRARY = ROOT / "ModelLibrary"
OUTPUT = ROOT / "Docs/ModelGallery"
INDIVIDUAL = OUTPUT / "individual"
WIDTH, HEIGHT = 480, 400
BG = (239, 243, 247)


def safe_name(path: Path) -> str:
    rel = path.relative_to(LIBRARY).with_suffix("").as_posix()
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", rel.replace("/", "--"))


def geometry_face_colors(geometry) -> np.ndarray:
    """Resolve UV textures first, then vertex/face colors, to color each triangle."""
    count = len(geometry.faces)
    visual = geometry.visual
    material = getattr(visual, "material", None)
    factor = getattr(material, "baseColorFactor", None)
    texture = getattr(material, "baseColorTexture", None)
    uv = getattr(visual, "uv", None)
    if texture is not None and uv is not None:
        tex = np.asarray(texture.convert("RGB") if hasattr(texture, "convert") else texture, dtype=np.uint8)
        uv = np.asarray(uv)
        if tex.ndim == 3 and len(uv) == len(geometry.vertices):
            uv_faces = uv[np.asarray(geometry.faces)].mean(axis=1)
            tx = np.clip((uv_faces[:, 0] % 1.0) * (tex.shape[1] - 1), 0, tex.shape[1] - 1).astype(int)
            ty = np.clip((1.0 - (uv_faces[:, 1] % 1.0)) * (tex.shape[0] - 1), 0, tex.shape[0] - 1).astype(int)
            sampled = tex[ty, tx, :3]
            if factor is not None:
                sampled = (sampled.astype(float) * (np.asarray(factor[:3]) / 255.0)).clip(0, 255).astype(np.uint8)
            return sampled
    if hasattr(visual, "face_colors"):
        try:
            colors = np.asarray(visual.face_colors, dtype=np.uint8)
            if colors.shape[0] == count:
                return colors[:, :3]
        except Exception:
            pass
    default = np.array([155, 165, 175, 255], dtype=np.uint8) if factor is None else np.asarray(factor, dtype=np.uint8)
    return np.tile(default[:3], (count, 1))


def render(path: Path) -> tuple[Path, int]:
    scene = trimesh.load(path, force="scene", process=False)
    vertex_parts, face_parts, color_parts = [], [], []
    offset = 0
    for node in scene.graph.nodes_geometry:
        transform, geometry_name = scene.graph[node]
        geometry = scene.geometry[geometry_name].copy()
        geometry.apply_transform(transform)
        vertices_part = np.asarray(geometry.vertices, dtype=np.float64)
        faces_part = np.asarray(geometry.faces, dtype=np.int64)
        if not len(faces_part):
            continue
        vertex_parts.append(vertices_part)
        face_parts.append(faces_part + offset)
        color_parts.append(geometry_face_colors(geometry))
        offset += len(vertices_part)
    if not face_parts:
        raise ValueError("scene has no triangle geometry")
    vertices = np.concatenate(vertex_parts)
    faces = np.concatenate(face_parts)
    face_colors = np.concatenate(color_parts)
    bounds = np.stack((vertices.min(axis=0), vertices.max(axis=0)))
    extents = bounds[1] - bounds[0]
    center = (bounds[0] + bounds[1]) * 0.5
    vertices -= center

    # Kenney and Quaternius use Y-up; original procedural Gridlock models use Z-up.
    use_y_up = "Community/Kenney/" in path.as_posix() or "Clothing/Quaternius/" in path.as_posix()
    if not use_y_up:
        y_floor = abs(float(bounds[0, 1])) <= max(float(extents[1]) * 0.08, 0.01)
        z_floor = abs(float(bounds[0, 2])) <= max(float(extents[2]) * 0.08, 0.01)
        use_y_up = y_floor and not z_floor
    up = np.array([0.0, 1.0, 0.0]) if use_y_up else np.array([0.0, 0.0, 1.0])
    toward_camera = np.array([2.7, 3.8, 2.8] if use_y_up else [2.7, -3.8, 2.8], dtype=np.float64)
    toward_camera /= np.linalg.norm(toward_camera)
    right = np.cross(up, toward_camera)
    right /= np.linalg.norm(right)
    camera_up = np.cross(toward_camera, right)
    projected_x = vertices @ right
    projected_y = vertices @ camera_up
    depth = vertices @ toward_camera
    extent = max(float(np.ptp(projected_x)), float(np.ptp(projected_y)), 1e-8)
    scale = min((WIDTH - 88) / extent, (HEIGHT - 112) / extent)
    px = (WIDTH * 0.5 + projected_x * scale).astype(np.int32)
    py = (HEIGHT * 0.53 - projected_y * scale).astype(np.int32)
    projected = np.column_stack((px, py))

    triangles = vertices[faces]
    cross = np.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0])
    normals = cross / np.maximum(np.linalg.norm(cross, axis=1, keepdims=True), 1e-12)
    light = np.array([-0.35, 0.65, 0.72] if use_y_up else [-0.35, -0.6, 0.72], dtype=np.float64)
    light /= np.linalg.norm(light)
    diffuse = np.clip(normals @ light, 0.0, 1.0)
    shade = (0.36 + 0.64 * diffuse)[:, None]
    colors = np.clip(face_colors.astype(np.float64) * shade, 0, 255).astype(np.uint8)
    face_depth = depth[faces].mean(axis=1)
    order = np.argsort(face_depth, kind="stable")

    image = Image.new("RGB", (WIDTH, HEIGHT), BG)
    draw = ImageDraw.Draw(image)
    # A subtle studio floor band frames each model without pretending to be a game screenshot.
    draw.rectangle((0, HEIGHT - 42, WIDTH, HEIGHT), fill=(226, 232, 238))
    for idx in order:
        polygon = [tuple(projected[v]) for v in faces[idx]]
        draw.polygon(polygon, fill=tuple(int(c) for c in colors[idx]))

    rel = path.relative_to(LIBRARY).as_posix()
    count = len(faces)
    font = ImageFont.load_default()
    draw.rectangle((0, 0, WIDTH, 42), fill=(251, 252, 253))
    draw.text((12, 9), rel[:62], fill=(28, 40, 52), font=font)
    draw.text((12, HEIGHT - 30), "{} triangles  |  offline CPU preview".format(f"{count:,}"),
              fill=(55, 70, 84), font=font)
    out = INDIVIDUAL / (safe_name(path) + ".png")
    out.parent.mkdir(parents=True, exist_ok=True)
    image.save(out, optimize=True)
    return out, count


def main() -> None:
    files = sorted(LIBRARY.rglob("*.glb"))
    if not files:
        raise RuntimeError("No GLB files found under {}".format(LIBRARY))
    OUTPUT.mkdir(parents=True, exist_ok=True)
    cards = []
    failures = []
    for index, path in enumerate(files, 1):
        try:
            image_path, triangles = render(path)
            with Image.open(image_path) as image:
                cards.append((image.copy(), path.relative_to(LIBRARY).as_posix(), triangles, image_path))
            print("[{}/{}] {} — {:,} triangles".format(index, len(files), path.relative_to(LIBRARY), triangles))
        except Exception as error:
            failures.append((path, str(error)))
            print("[{}/{}] FAILED {}: {}".format(index, len(files), path.relative_to(LIBRARY), error))

    columns, rows, per_sheet = 4, 4, 16
    sheet_paths = []
    for batch_index in range(0, len(cards), per_sheet):
        batch = cards[batch_index:batch_index + per_sheet]
        card_w, card_h = 320, 276
        sheet = Image.new("RGB", (columns * card_w, rows * card_h), (218, 225, 232))
        draw = ImageDraw.Draw(sheet)
        for slot, (image, rel, triangles, _) in enumerate(batch):
            x, y = (slot % columns) * card_w, (slot // columns) * card_h
            thumb = image.resize((card_w, 230), Image.Resampling.LANCZOS)
            sheet.paste(thumb, (x, y))
            label = rel if len(rel) <= 45 else "…" + rel[-44:]
            draw.text((x + 7, y + 234), label, fill=(26, 38, 50))
            draw.text((x + 7, y + 252), "{:,} triangles".format(triangles), fill=(58, 73, 88))
        sheet_path = OUTPUT / "contact_sheet_{:02d}.png".format(len(sheet_paths) + 1)
        sheet.save(sheet_path, optimize=True)
        sheet_paths.append(sheet_path)

    rows_html = []
    for image, rel, triangles, image_path in cards:
        src = "individual/" + image_path.name
        rows_html.append(
            '<article><a href="{}"><img loading="lazy" src="{}" alt="{}"></a>'
            '<div><strong>{}</strong><br>{:,} triangles</div></article>'.format(
                html.escape(src), html.escape(src), html.escape(rel), html.escape(rel), triangles))
    failure_html = "" if not failures else "<h2>Render failures</h2><ul>{}</ul>".format(
        "".join("<li>{}: {}</li>".format(html.escape(p.relative_to(LIBRARY).as_posix()), html.escape(e))
                for p, e in failures))
    index_html = """<!doctype html><meta charset="utf-8"><title>Gridlock GLB model previews</title>
<style>body{font:14px system-ui;background:#e9eef3;color:#172431;margin:24px}h1{margin-bottom:4px}
.note{color:#4b5e70;margin-bottom:20px}main{display:grid;grid-template-columns:repeat(auto-fill,minmax(240px,1fr));gap:12px}
article{background:#fff;border-radius:8px;overflow:hidden;box-shadow:0 2px 8px #17243122}img{display:block;width:100%;height:auto}
article div{padding:8px;overflow-wrap:anywhere;font-size:12px}</style>
<h1>Gridlock model previews</h1><p class="note">__COUNT__ individual offline CPU renders from __TOTAL__ GLBs. These are not Unreal Engine screenshots and do not certify production quality.</p>
<nav>__NAV__</nav><main>__ROWS__</main>__FAIL__"""
    navigation = " · ".join('<a href="{}">Contact sheet {}</a>'.format(
        html.escape(p.name), i + 1) for i, p in enumerate(sheet_paths))
    index_html = (index_html.replace("__COUNT__", str(len(cards)))
                  .replace("__TOTAL__", str(len(files)))
                  .replace("__NAV__", navigation)
                  .replace("__ROWS__", "".join(rows_html))
                  .replace("__FAIL__", failure_html))
    (OUTPUT / "index.html").write_text(index_html, encoding="utf-8")
    print("Rendered {}/{} models; {} failures. Gallery: {}".format(
        len(cards), len(files), len(failures), OUTPUT / "index.html"))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
