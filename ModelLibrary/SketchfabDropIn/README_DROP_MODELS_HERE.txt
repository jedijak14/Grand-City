DROP DOWNLOADED MODELS IN THIS FOLDER
=====================================
1. Download free models (Sketchfab "Downloadable" filter, Fab free section, etc.).
   Prefer .glb or .fbx. Check the license (CC0 / CC-BY are safest) and keep credit in CREDITS.txt.
2. Put the files here:  ModelLibrary/SketchfabDropIn/
3. In Unreal Editor run:  Tools > Execute Python Script > Tools/ImportModels_Editor.py
4. Create a Blueprint child of the matching C++ class and assign the mesh:
     Cars    -> BP child of GCVehiclePawn   -> BodyMesh (+ WheelMesh0..3, adjust Wheels offsets)
     Player  -> BP child of GCCharacter     -> Mesh (skeletal) + AnimBP (parent: GCAnimInstance)
     Peds    -> BP child of GCPedestrian    -> Mesh (skeletal)
     Plane   -> BP child of GCPlanePawn     -> FuselageMesh (+ PropellerMesh)
     Boat    -> BP child of GCBoatPawn      -> HullMesh
5. For car BODY meshes: in the mesh asset enable "Allow CPU Access" then re-save,
   otherwise crash deformation will (by design) fall back to a rigid body.
