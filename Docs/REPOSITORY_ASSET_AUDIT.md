# Repository asset-source audit

Reviewed 2026-10-23 for Gridlock's production model replacement effort. These repositories are discovery sources, not blanket licenses for every file. Nothing from these three repositories was copied into `ModelLibrary` in this review.

## Khronos glTF Sample Assets

- Repository: [KhronosGroup/glTF-Sample-Assets](https://github.com/KhronosGroup/glTF-Sample-Assets), checked at `main` commit `8f492390ddc7a2ae221a1369ef560589cf8c70fb`.
- **Car Concept** (`Models/CarConcept/GLB/CarConcept.glb`): inspected the repository's large screenshot and the GLB scene metadata. 213,347 triangles, 97 mesh objects, 29 materials; multiple detailed interior/exterior components, paint variants, and wheel/brake geometry. The model and textures are CC BY 4.0, attributed to Eric Chadwick / Darmstadt Graphics Group GmbH. Model metadata separately identifies Khronos and 3D Commerce logos as trademarks, excluded from the CC license. This is the best high-detail vehicle lead in the three repositories, but the supplied file is not approved for shipping unchanged: remove/replace all excluded logo/trademark content and inspect the modified model, preserve attribution, then test hierarchy, materials, scale, collision, and vehicle integration in UE 5.8. No copy/import/runtime verification has been performed.
- **Virtual City**: 8,383 triangles across 135 meshes; not a high-density replacement. Its model files are covered by a 3DRT testing license with allowances for glTF testing, not a general production-use license. Rejected for Gridlock.
- **Sponza**: architectural interior but licensed under the Cryengine Limited License Agreement; not adopted or treated as freely reusable.
- Repository-level presence does not override these per-model license files. The Khronos repository is primarily a conformance/sample collection, not a production city asset pack.

## BabylonJS Assets

- Repository: [BabylonJS/Assets](https://github.com/BabylonJS/Assets), checked at `master` commit `d96d7ae135a072725902d3dafc448b47d50c1e79`. The repository's overall attribution license is CC BY 4.0, with individual asset exceptions and source-specific license files that still require review.
- **Gothic Cloister Corner**: inspected both full and mobile GLBs; each contains 1,013,385 triangles across 16 meshes. Per-asset `license.txt` names Zoilo / iliedom and grants CC BY 4.0. This is a detailed Gothic architectural corner, not a suitable modern-city/airport building replacement; its large footprint/file weight and material/LOD/collision behavior also remain untested in Unreal. Not adopted.
- **house_scene.glb**, **valleyvillage.glb**, and **village.glb**: inspected scene accessors; 26, 1,258, and 458 triangles respectively. They are lightweight Babylon demo scenes and do not meet the current detail screening bar.
- Babylon.js includes some substantial visual assets, but is not a curated high-quality contemporary city model library. Do not treat the repository's top-level CC BY as proof that every bundled third-party asset is covered by it.

## Godot demo projects

- Repository: [godotengine/godot-demo-projects](https://github.com/godotengine/godot-demo-projects), checked at `master` commit `b761b4cd5718a7118f2c95eb55f181d0a5f004f1`.
- The repository is a collection of engine demos, not a production asset set. Representative bundled models include demo-scale characters, props, and town scenes; several are low-density. The tree also contains per-asset license sidecars and external-origin credits, so the top-level MIT license is not assumed to cover every model or texture.
- No model from this repository was selected as a higher-quality Gridlock replacement in this pass.

## Result and next action

This audit verifies one promising detailed vehicle lead (Khronos Car Concept), but it has a trademark/logo clearance step and has not been integrated. None of the three repositories supplied a verified, production-suitable replacement set for the project's buildings, aircraft, vehicles, airport, and interiors. Continue sourcing category-specific assets; do not leave low-density models approved as final solely because these demo repositories contain convenient files. Unreal Engine 5.8 import and visual/runtime validation remain outstanding.
