"""Generate an original detailed Coastline Airways terminal GLB for Gridlock.

Requires Tools/requirements-models.txt. Units are centimetres; X is terminal width,
Y is landside-to-airside, Z is up. The source model is procedural original geometry,
not a photogrammetric or real-airport replica.
"""
from __future__ import annotations
import math
from pathlib import Path
import numpy as np
import trimesh
from trimesh.visual.material import PBRMaterial
from trimesh.visual.texture import TextureVisuals

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "ModelLibrary/PublicGLB/gridlock_airport_terminal.glb"
MAT = {
    "concrete": PBRMaterial(name="Warm pale concrete", baseColorFactor=[174, 181, 179, 255], metallicFactor=0.05, roughnessFactor=0.83),
    "stone": PBRMaterial(name="Airport limestone panels", baseColorFactor=[215, 219, 212, 255], metallicFactor=0.0, roughnessFactor=0.72),
    "glass": PBRMaterial(name="Blue-grey curtain glass", baseColorFactor=[54, 105, 126, 205], metallicFactor=0.18, roughnessFactor=0.16, alphaMode="BLEND"),
    "frame": PBRMaterial(name="Anodized graphite frames", baseColorFactor=[32, 47, 56, 255], metallicFactor=0.78, roughnessFactor=0.28),
    "steel": PBRMaterial(name="Brushed structural steel", baseColorFactor=[116, 139, 145, 255], metallicFactor=0.87, roughnessFactor=0.31),
    "navy": PBRMaterial(name="Coastline deep navy", baseColorFactor=[15, 37, 65, 255], metallicFactor=0.22, roughnessFactor=0.4),
    "teal": PBRMaterial(name="Coastline sea teal", baseColorFactor=[18, 137, 145, 255], metallicFactor=0.28, roughnessFactor=0.3),
    "gold": PBRMaterial(name="Coastline muted gold", baseColorFactor=[218, 169, 74, 255], metallicFactor=0.72, roughnessFactor=0.28),
    "dark": PBRMaterial(name="Rubber and dark trim", baseColorFactor=[27, 31, 34, 255], metallicFactor=0.08, roughnessFactor=0.76),
    "seat": PBRMaterial(name="Lounge seat blue", baseColorFactor=[39, 76, 105, 255], metallicFactor=0.08, roughnessFactor=0.64),
    "floor": PBRMaterial(name="Polished terrazzo", baseColorFactor=[203, 205, 198, 255], metallicFactor=0.06, roughnessFactor=0.27),
    "display": PBRMaterial(name="Display black", baseColorFactor=[10, 19, 26, 255], metallicFactor=0.22, roughnessFactor=0.15),
    "light": PBRMaterial(name="Warm interior lighting", baseColorFactor=[255, 228, 173, 255], metallicFactor=0.0, roughnessFactor=0.25, emissiveFactor=[0.45, 0.28, 0.10]),
}
SCENE = trimesh.Scene()
PARTS = {key: [] for key in MAT}

def add(mesh, material, name):
    if mesh is None or not len(mesh.faces): return
    mesh.remove_unreferenced_vertices()
    PARTS[material].append(mesh)

def box(name, center, size, material, rotation=None):
    mesh = trimesh.creation.box(extents=np.asarray(size, dtype=float))
    if rotation is not None: mesh.apply_transform(rotation)
    mesh.apply_translation(center)
    add(mesh, material, name)
    return mesh

def cylinder(name, start, end, radius, material, sections=16):
    a, b = np.asarray(start, float), np.asarray(end, float)
    vector = b - a; length = float(np.linalg.norm(vector))
    if length <= 0.01: return
    mesh = trimesh.creation.cylinder(radius=radius, height=length, sections=sections)
    rot = trimesh.geometry.align_vectors([0,0,1], vector / length)
    if rot is not None: mesh.apply_transform(rot)
    mesh.apply_translation((a+b)*0.5)
    add(mesh, material, name)

FONT = {
'A':('01110','10001','10001','11111','10001','10001','10001'),'B':('11110','10001','10001','11110','10001','10001','11110'),
'C':('01111','10000','10000','10000','10000','10000','01111'),'D':('11110','10001','10001','10001','10001','10001','11110'),
'E':('11111','10000','10000','11110','10000','10000','11111'),'F':('11111','10000','10000','11110','10000','10000','10000'),
'G':('01111','10000','10000','10111','10001','10001','01111'),'H':('10001','10001','10001','11111','10001','10001','10001'),
'I':('11111','00100','00100','00100','00100','00100','11111'),'J':('00111','00010','00010','00010','10010','10010','01100'),
'K':('10001','10010','10100','11000','10100','10010','10001'),'L':('10000','10000','10000','10000','10000','10000','11111'),
'M':('10001','11011','10101','10101','10001','10001','10001'),'N':('10001','11001','10101','10011','10001','10001','10001'),
'O':('01110','10001','10001','10001','10001','10001','01110'),'P':('11110','10001','10001','11110','10000','10000','10000'),
'Q':('01110','10001','10001','10001','10101','10010','01101'),'R':('11110','10001','10001','11110','10100','10010','10001'),
'S':('01111','10000','10000','01110','00001','00001','11110'),'T':('11111','00100','00100','00100','00100','00100','00100'),
'U':('10001','10001','10001','10001','10001','10001','01110'),'V':('10001','10001','10001','10001','10001','01010','00100'),
'W':('10001','10001','10001','10101','10101','10101','01010'),'X':('10001','10001','01010','00100','01010','10001','10001'),
'Y':('10001','10001','01010','00100','00100','00100','00100'),'Z':('11111','00001','00010','00100','01000','10000','11111'),
'0':('01110','10001','10011','10101','11001','10001','01110'),'1':('00100','01100','00100','00100','00100','00100','01110'),
'2':('01110','10001','00001','00010','00100','01000','11111'),'3':('11110','00001','00001','00110','00001','00001','11110'),
'4':('00010','00110','01010','10010','11111','00010','00010'),'5':('11111','10000','10000','11110','00001','00001','11110'),
'6':('01110','10000','10000','11110','10001','10001','01110'),'7':('11111','00001','00010','00100','01000','01000','01000'),
'8':('01110','10001','10001','01110','10001','10001','01110'),'9':('01110','10001','10001','01111','00001','00001','01110'),
'-':('00000','00000','00000','11111','00000','00000','00000'),':':('00000','00100','00100','00000','00100','00100','00000'),' ':('00000',)*7
}

def text_pixels(text, origin, pixel=5.0, face_y=0.0, material="light"):
    meshes=[]; x0,z0=origin; cursor=0.0
    for ch in text.upper():
        pattern=FONT.get(ch,FONT[' '])
        for row,line in enumerate(pattern):
            for col,bit in enumerate(line):
                if bit=='1':
                    m=trimesh.creation.box(extents=[pixel*0.8, 3.0, pixel*0.8])
                    m.apply_translation((x0+cursor+col*pixel,face_y,z0+(6-row)*pixel)); meshes.append(m)
        cursor += 6*pixel
    if meshes: add(trimesh.util.concatenate(meshes),material,"Sign lettering - "+text)

def seat(name, x, y):
    box(name+" frame",(x,y,42),(105,62,8),"frame")
    box(name+" cushion",(x,y,60),(99,58,22),"seat")
    box(name+" back",(x,y+27,95),(98,12,54),"seat")
    for dx in (-42,42):
        cylinder(name+" leg",(x+dx,y,8),(x+dx,y,52),3.0,"steel",10)

def main():
    # Broad floor slab, glazed concourse and structural roof form the outer shell.
    box("Terminal foundation",(0,0,-16),(3400,2200,32),"concrete")
    box("Terrazzo public hall",(0,0,2),(3300,2100,8),"floor")
    # Airside glass curtain wall with fine mullions and three accessible gate openings.
    for bay in range(16):
        x=-1500+bay*200
        if bay in (6,7,8,9): continue
        box("Airside glazing",(x,1025,360),(184,15,470),"glass")
        box("Curtain wall mullion",(x+94,1012,360),(10,24,490),"frame")
    for z in (125,595): box("Curtain wall transom",(0,1010,z),(3180,26,10),"frame")
    # Side elevations: limestone piers alternate with glazed windows.
    for side in (-1,1):
        x=side*1625
        for bay in range(8):
            y=-820+bay*230
            box("Side limestone pier",(x,y,310),(38,42,610),"stone")
            box("Side clerestory glass",(x-side*14,y,465),(12,175,150),"glass")
        box("Side wall base",(x,0,72),(50,2100,80),"stone")
    # Landside frontage with automatic door banks, clear glazing, and a long weather canopy.
    for bay in range(14):
        x=-1400+bay*210
        if bay in (5,6,7,8):
            box("Entrance glass door",(x,-1025,150),(186,18,300),"glass")
            box("Sliding door stile",(x-90,-1008,150),(8,24,305),"frame")
        else:
            box("Landside glazed bay",(x,-1025,270),(184,18,490),"glass")
        box("Landside mullion",(x+98,-1008,270),(10,25,520),"frame")
    for x in range(-1550,1551,250):
        cylinder("Canopy column",(x,-1170,0),(x,-1170,300),10,"steel",16)
    box("Passenger dropoff canopy",(0,-1185,315),(3300,330,32),"frame")
    box("Canopy fascia",(0,-1345,300),(3300,18,45),"teal")
    # Roof: layered parapet, steel portal trusses, skylights and clerestory spine.
    box("Main roof deck",(0,0,680),(3400,2200,60),"stone")
    box("Roof fascia - navy",(0,-15,632),(3450,38,38),"navy")
    box("Roof fascia - teal band",(0,-38,602),(3450,16,18),"teal")
    box("Central skylight",(0,120,716),(1750,510,12),"glass")
    for x in range(-800,801,200):
        box("Skylight rib",(x,120,732),(12,520,24),"steel")
    for y in range(-820,901,340):
        for x in (-1450,1450):
            box("Roof truss column",(x,y,340),(28,28,620),"steel")
        box("Roof portal truss",(0,y,625),(2900,24,34),"steel")
    # Entrance canopy branded signage.
    box("Coastline Airways fascia",(0,-1360,390),(1080,20,126),"navy")
    box("Fascia teal stripe",(0,-1374,333),(1080,10,10),"teal")
    text_pixels("COASTLINE",(-285,390),pixel=14,face_y=-1377,material="light")
    text_pixels("AIRWAYS",(-155,320),pixel=10,face_y=-1378,material="gold")
    # Departures hall check-in counter island: stone plinth, teal desk fronts, monitors, kiosks.
    for i in range(7):
        x=-1250+i*410
        box("Check-in counter plinth",(x,-560,44),(300,120,82),"navy")
        box("Check-in counter worktop",(x,-560,91),(320,142,16),"stone")
        box("Counter teal identity strip",(x,-630,55),(268,10,14),"teal")
        box("Self service screen body",(x,-520,157),(90,22,104),"frame")
        box("Self service screen glass",(x,-507,164),(78,5,74),"display")
    # Queue stanchions, security lanes, arch frames, X-ray belt and tray stack.
    for lane in range(3):
        x=850+lane*215
        box("Security conveyor",(x,-80,46),(120,360,72),"frame")
        box("Conveyor belt",(x,-80,84),(106,335,8),"dark")
        for side in (-1,1):
            cylinder("Metal detector upright",(x+side*78,145,0),(x+side*78,145,215),8,"steel",12)
        box("Metal detector header",(x,145,215),(170,18,20),"teal")
        for post_y in (-370,-220):
            for side in (-1,1):
                cylinder("Queue stanchion",(x+side*145,post_y,0),(x+side*145,post_y,94),4,"steel",12)
            cylinder("Queue belt",(x-145,post_y,90),(x+145,post_y,90),2,"navy",8)
    # Gate lounge benches in realistic groups around an open central circulation aisle.
    for row in range(3):
        y=360+row*220
        for i in range(7):
            x=-1370+i*420
            if 2<=i<=3: continue
            seat("Airport lounge chair",x,y)
    # Gates, podiums, electronic flight boards and bridge-side portals.
    for i,gate in enumerate(("A3","A4")):
        x=-460+i*920
        box("Gate podium navy",(x,850,74),(220,86,145),"navy")
        box("Gate podium teal face",(x,804,80),(184,8,76),"teal")
        box("Gate monitor",(x,905,185),(280,22,190),"display")
        text_pixels("GATE "+gate,(x-118,165),pixel=8,face_y=919,material="light")
        box("Jet bridge portal frame",(x,1040,245),(310,46,390),"frame")
        box("Jet bridge portal glass",(x,1012,245),(265,12,342),"glass")
        box("Gate number sign",(x,770,355),(280,12,70),"navy")
        text_pixels("GATE "+gate,(x-96,338),pixel=7,face_y=762,material="gold")
    box("Departures board housing",(0,350,510),(980,28,240),"display")
    text_pixels("DEPARTURES",(-405,495),pixel=12,face_y=334,material="light")
    text_pixels("GCI  GATES A3 A4",(-315,395),pixel=8,face_y=333,material="gold")
    # Arrival/baggage service island and carousel belt with safety edging.
    box("Baggage service counter",(-1280,710,75),(450,150,145),"navy")
    box("Baggage desk stone top",(-1280,710,155),(476,170,18),"stone")
    box("Baggage belt frame",(-790,720,58),(470,175,82),"dark")
    box("Baggage belt rubber",(-790,720,104),(430,145,12),"frame")
    for i in range(5):
        box("Baggage carousel roller",(-970+i*90,720,112),(9,155,8),"steel")
    # Restroom/service core with separate wall planes and door openings.
    box("Restroom core exterior",(1330,570,190),(350,520,380),"stone")
    box("Restroom entry recess",(1140,310,170),(10,170,320),"frame")
    for i in range(3):
        box("Restroom stall partition",(1250+i*95,690,105),(8,170,210),"stone")
        box("Restroom stall door",(1290+i*95,610,115),(72,10,180),"frame")
    # Decorative planters, low palms and seating-side lighting.
    for x in (-1450,1450):
        for y in (-500,250,700):
            box("Planter stone",(x,y,30),(220,160,60),"concrete")
            cylinder("Planter trunk",(x,y,60),(x,y,245),10,"frame",12)
            for k in range(6):
                ang=k*math.tau/6
                cylinder("Palm frond",(x,y,210),(x+math.cos(ang)*80,y+math.sin(ang)*80,300),5,"teal",8)
    for x in range(-1250,1251,500):
        box("Recessed warm ceiling light",(x,0,586),(280,80,7),"light")
    # Gate-side inner partition height creates a readable gate zone but keeps sightlines open.
    for x in (-900,900):
        box("Lounge divider base",(x,260,76),(24,780,145),"navy")
        box("Lounge divider glass",(x,260,255),(12,780,220),"glass")
        for y in range(-100,651,150): box("Divider mullion",(x-10,y,255),(18,12,220),"steel")
    # Merge components by material to keep the imported terminal to a small number of draw calls.
    for index, (material, meshes) in enumerate(PARTS.items()):
        if not meshes: continue
        merged = trimesh.util.concatenate(meshes)
        merged.visual = TextureVisuals(material=MAT[material])
        name = "Terminal_{}_LOD0".format(material)
        SCENE.add_geometry(merged, geom_name=name, node_name=name)
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    glb=SCENE.export(file_type="glb")
    OUTPUT.write_bytes(glb)
    print(f"Wrote {OUTPUT} ({len(glb):,} bytes, {len(SCENE.geometry)} material-batched meshes)")

if __name__ == "__main__": main()
