"""Generate original detailed dynamic escalator tread and static side-rail GLBs for Gridlock.

Requires Tools/requirements-models.txt. The step is authored in a 100 cm cube so it can
replace the escalator actor's Engine Cube while preserving its existing dimension controls.
"""
from __future__ import annotations
import math
from pathlib import Path
import numpy as np
import trimesh
from trimesh.visual.material import PBRMaterial
from trimesh.visual.texture import TextureVisuals

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"ModelLibrary/PublicGLB"
MATS={
 "tread":PBRMaterial(name="Anodized ribbed tread",baseColorFactor=[126,140,146,255],metallicFactor=.88,roughnessFactor=.34),
 "frame":PBRMaterial(name="Escalator graphite frame",baseColorFactor=[34,42,49,255],metallicFactor=.63,roughnessFactor=.39),
 "steel":PBRMaterial(name="Escalator stainless steel",baseColorFactor=[172,181,182,255],metallicFactor=.91,roughnessFactor=.24),
 "glass":PBRMaterial(name="Escalator safety glass",baseColorFactor=[105,174,185,92],metallicFactor=.1,roughnessFactor=.12,alphaMode="BLEND"),
 "gold":PBRMaterial(name="Escalator safety line",baseColorFactor=[236,178,49,255],metallicFactor=.16,roughnessFactor=.42),
 "board":PBRMaterial(name="Surfboard sea-glass teal",baseColorFactor=[14,133,148,255],metallicFactor=.26,roughnessFactor=.23),
 "boardstripe":PBRMaterial(name="Surfboard ivory pinstripe",baseColorFactor=[235,224,193,255],metallicFactor=.06,roughnessFactor=.3),
}

def build_step():
    # Base is 100 x 100 x 84 cm, then 16 individual transverse ridges fill the remaining envelope.
    parts=[]
    base=trimesh.creation.box(extents=[100,100,84]);base.apply_translation([0,0,-8]);parts.append(base)
    for i in range(16):
        x=-46+i*(92/15)
        rib=trimesh.creation.box(extents=[3.2,96,12]);rib.apply_translation([x,0,44]);parts.append(rib)
    nose=trimesh.creation.box(extents=[100,6,10]);nose.apply_translation([0,-47,45]);parts.append(nose)
    mesh=trimesh.util.concatenate(parts)
    mesh.visual=TextureVisuals(material=MATS["tread"])
    scene=trimesh.Scene();scene.add_geometry(mesh,geom_name="Ribbed_Escalator_Tread",node_name="Ribbed_Escalator_Tread")
    (OUT/"gridlock_escalator_step.glb").write_bytes(scene.export(file_type="glb"))

def add(parts,mesh,material,name):
    parts[material].append(mesh)

def railings():
    scene=trimesh.Scene()
    parts={key:[] for key in MATS}
    # Side panels and lower steel skirts follow the actual 700 cm, 350 cm incline.
    slope=math.atan2(350,700); rot=trimesh.transformations.rotation_matrix(-slope,[0,1,0])
    for side in (-1,1):
        y=side*116
        for i in range(5):
            x=70+i*140; z=x*0.5+52
            panel=trimesh.creation.box(extents=[148,12,118]);panel.apply_transform(rot);panel.apply_translation([x,y,z])
            add(parts,panel,"glass",f"Safety_Glass_{side}_{i}")
            upright=trimesh.creation.box(extents=[12,20,164]);upright.apply_translation([x,y, x*0.5+60])
            add(parts,upright,"frame",f"Side_Rail_Post_{side}_{i}")
        # Continuous rounded handrail with a bright stainless under-rail and dark belt core.
        for offset,material,radius in ((150,"frame",13),(126,"steel",5)):
            start=np.array([0.0,y,float(offset)]);end=np.array([700.0,y,350.0+float(offset)]);vec=end-start
            beam=trimesh.creation.cylinder(radius=radius,height=float(np.linalg.norm(vec)),sections=16)
            align=trimesh.geometry.align_vectors([0,0,1],vec/np.linalg.norm(vec))
            if align is not None:beam.apply_transform(align)
            beam.apply_translation((start+end)*.5)
            add(parts,beam,material,f"Handrail_{side}_{material}")
        for x in (10,690):
            base_z=x*.5
            for zoff in (38,78,118):
                edge=trimesh.creation.box(extents=[38,28,8]);edge.apply_translation([x,y*1.08,base_z+zoff])
                add(parts,edge,"gold",f"Safety_Edge_{side}_{x}_{zoff}")
    for material,meshes in parts.items():
        if not meshes: continue
        merged=trimesh.util.concatenate(meshes)
        merged.visual=TextureVisuals(material=MATS[material])
        name="Escalator_Railing_"+material
        scene.add_geometry(merged,geom_name=name,node_name=name)
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/"gridlock_escalator_railings.glb").write_bytes(scene.export(file_type="glb"))

def cylinder_between(start,end,radius,sections=16):
    start=np.asarray(start,float);end=np.asarray(end,float);direction=end-start
    length=float(np.linalg.norm(direction))
    mesh=trimesh.creation.cylinder(radius=radius,height=length,sections=sections)
    align=trimesh.geometry.align_vectors([0,0,1],direction/length)
    if align is not None:mesh.apply_transform(align)
    mesh.apply_translation((start+end)*0.5)
    return mesh

def elevator_guide_frame():
    scene=trimesh.Scene();parts={key:[] for key in MATS};extent=230.0;height=3000.0
    corners=[(x,y) for x in (-extent,extent) for y in (-extent,extent)]
    # Four structural rails with base shoes and cap plates.
    for x,y in corners:
        add(parts,cylinder_between([x,y,0],[x,y,height],10),"steel",f"Guide_Post_{x}_{y}")
        for z in (25,height-25):
            plate=trimesh.creation.box(extents=[92,92,24]);plate.apply_translation([x,y,z])
            add(parts,plate,"frame",f"Guide_Base_{x}_{y}_{z}")
    # Outside cross-bracing stabilizes the tower while leaving an unobstructed car envelope.
    for face_x in (-extent,extent):
        for z0,z1 in ((80,1480),(1520,2920)):
            add(parts,cylinder_between([face_x,-extent,z0],[face_x,extent,z1],7),"frame",f"X_Brace_{face_x}_{z0}")
            add(parts,cylinder_between([face_x,extent,z0],[face_x,-extent,z1],7),"frame",f"X_Brace_{face_x}_{z0}_b")
    for face_y in (-extent,extent):
        for z in (0,height):
            for x in (-extent,extent):
                add(parts,cylinder_between([x,face_y,z],[0,face_y,z],7),"frame",f"Cross_Tie_{face_y}_{x}_{z}")
    # Polished inner guide tracks and roller shoes remain just outside the 300 cm cabin envelope.
    for x in (-174,174):
        add(parts,cylinder_between([x,0,30],[x,0,height-30],7),"steel",f"Cab_Guide_{x}")
        for z in range(250,2851,500):
            shoe=trimesh.creation.box(extents=[36,66,32]);shoe.apply_translation([x,0,z])
            add(parts,shoe,"gold",f"Guide_Roller_Shoe_{x}_{z}")
    # Maintenance ladder on the rear face, with safe cage hoops and anti-slip lower marking.
    for x in (-90,90): add(parts,cylinder_between([x,-extent,100],[x,-extent,2800],5),"steel",f"Service_Ladder_Rail_{x}")
    for z in range(150,2801,200): add(parts,cylinder_between([-90,-extent,z],[90,-extent,z],4),"steel",f"Service_Ladder_Rung_{z}")
    for material,meshes in parts.items():
        if not meshes: continue
        merged=trimesh.util.concatenate(meshes);merged.visual=TextureVisuals(material=MATS[material])
        name="Elevator_Guide_"+material;scene.add_geometry(merged,geom_name=name,node_name=name)
    (OUT/"gridlock_elevator_guide_frame.glb").write_bytes(scene.export(file_type="glb"))

def surfboard():
    parts={"board":[],"boardstripe":[]}
    stations=((-125,1.0,0.4),(-116,6.0,2.4),(-96,15.0,3.5),(-62,22.0,4.0),
              (-20,26.0,4.2),(35,27.5,4.2),(80,23.0,3.8),(108,15.0,3.2),(121,7.0,2.0),(125,1.0,0.4))
    sides=32;vertices=[]
    for x,width,thickness in stations:
        for j in range(sides):
            angle=math.tau*j/sides
            vertices.append([x,width*math.cos(angle),thickness*math.sin(angle)])
    faces=[]
    for ring in range(len(stations)-1):
        for j in range(sides):
            a=ring*sides+j;b=ring*sides+(j+1)%sides;c=(ring+1)*sides+j;d=(ring+1)*sides+(j+1)%sides
            faces.extend(([a,b,c],[b,d,c]))
    for end in (0,len(stations)-1):
        center=len(vertices);vertices.append([stations[end][0],0,0]);ring=end*sides
        for j in range(sides):
            faces.append([center,ring+j,ring+(j+1)%sides] if end==0 else [center,ring+(j+1)%sides,ring+j])
    board=trimesh.Trimesh(vertices=np.asarray(vertices),faces=np.asarray(faces),process=True)
    parts["board"].append(board)
    for y in (-2.7,0.0,2.7):
        stripe=trimesh.creation.box(extents=[208,0.8,0.32]);stripe.apply_translation([0,y,4.18])
        parts["boardstripe"].append(stripe)
    for y in (-10.0,0.0,10.0):
        fin=trimesh.creation.box(extents=[24,3.0,11.0]);fin.apply_translation([-88,y,-8.0])
        parts["board"].append(fin)
    scene=trimesh.Scene()
    for material,meshes in parts.items():
        merged=trimesh.util.concatenate(meshes);merged.visual=TextureVisuals(material=MATS[material])
        name="Surfboard_"+material;scene.add_geometry(merged,geom_name=name,node_name=name)
    (OUT/"gridlock_surfboard.glb").write_bytes(scene.export(file_type="glb"))
def main():
    OUT.mkdir(parents=True,exist_ok=True);build_step();railings();elevator_guide_frame();surfboard()
    for name in ("gridlock_escalator_step.glb","gridlock_escalator_railings.glb","gridlock_elevator_guide_frame.glb","gridlock_surfboard.glb"):
        print("Wrote",OUT/name,(OUT/name).stat().st_size,"bytes")
if __name__=="__main__":main()
