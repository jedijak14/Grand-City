# Sketchfab sourcing pass

Reviewed Sketchfab model pages on 2026-10-23 in response to the request to use Sketchfab. These are source leads, not imported/approved assets; all model-page download terms and the actual downloaded files must be checked before use. CC BY requires preserving the creator attribution and license notice. No Sketchfab files were downloaded in this pass because this workspace cannot reach Sketchfab's API/download hosts; `Tools/DownloadSketchfabModel.py` is provided for use in a local environment that can access Sketchfab.

## Best architecture lead: 38-building kit

**Terminal** by hnanw, Sketchfab model UID `935d92e43c4649a683777cc0d2bfe7fa`:
[model page](https://sketchfab.com/3d-models/terminal-935d92e43c4649a683777cc0d2bfe7fa)

The model page reports 340,900 triangles, textures, 1,773 scene objects, and a kit of 38 buildings plus 10 miscellaneous objects; it shows a Download 3D Model button and labels the asset CC Attribution. The author notes that there may be unnecessary vertices and identifies the work as a semi-futuristic kit. This is the best lead for broad city replacement coverage in this pass, subject to file inspection: check the actual geometry, texture quality, object names, source provenance of any included kit pieces, usable pivots, LOD/collision needs, and whether it fits Gridlock's art direction. It is not yet imported or verified.

## Additional leads

- **Old Residential Building [4k]** by Andrej Grave, UID `dd400ed693cf416e940c5b8e3107205e` ([page](https://sketchfab.com/3d-models/old-residential-building-4k-dd400ed693cf416e940c5b8e3107205e)): page reports 276,700 triangles, 4K textures, CC Attribution, and a Download button. Strong candidate for a worn residential/urban block, not a complete replacement set. LODs, collision, topology, and exact textures still need inspection.
- **Airbus A320** by Blender Studio, UID `d3b0dc06ae174031b87403144b15de61` ([page](https://sketchfab.com/3d-models/airbus-a320-d3b0dc06ae174031b87403144b15de61)): page reports 624,500 triangles, CC Attribution, and a Download button. Candidate for aircraft inspection, but verify actual geometry, controllable part separation, texture/material quality, and remove/replace any protected airline or manufacturer marks before Gridlock use.
- **Realistic Classic Sedan** by DevPoly3D, UID `c0f6b70538fc4864816239c764cf6dc7` ([page](https://sketchfab.com/3d-models/realistic-classic-sedan-unity-unreal-c0f6b70538fc4864816239c764cf6dc7)): page claims PBR 4K maps, interior details, game-ready topology, and CC Attribution; reports 1.1M triangles. This is likely over budget for a normal traffic car unless reduced and provided with LODs. Do not treat the page's game-ready claims as verified.

## Not selected

- **Airport Terminal** by Zeps3D reports 287,400 triangles, but the creator says it is available on Fab and the Sketchfab page has no download button. Not an available Sketchfab download.
- **A Minimalist Airport Diorama** by AGUNG.IHACKSTUFF has a 1M-triangle report, CC Attribution, and a download button, but it bundles branded JAL/Singapore Airlines aircraft and an Airbus plane. Avoid shipping it unchanged; any use would require inspection and removal/replacement of protected branding and aircraft content.

## Download and integration workflow

`Tools/DownloadSketchfabModel.py` accepts a Sketchfab model UID and retrieves the permitted GLB (or glTF archive) using `SKETCHFAB_API_TOKEN` from the local environment. It does not contain or persist a credential. It writes to `ModelLibrary/SketchfabDropIn/` and creates a `CREDITS.txt` that must be completed from the exact model page. The project importer already scans `SketchfabDropIn`.

After downloading, inspect each file's visuals, triangle count, material/texture channels, hierarchy, scale, LOD/collision requirements, brand marks, license and attribution, then generate a preview and verify UE 5.8 import. Do not replace the original/current model until those checks pass. This environment's current network access does not reach Sketchfab's API or binary-download hosts, so the downloader was syntax-checked but not run here.
