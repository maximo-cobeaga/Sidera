"""Non-final measured proxy for the main character. Run inside the live Bridge.

Creates only its own scene. Never imports or changes the Q1 humanoid assets.
The generated objects deliberately carry PROXY_ names and are not export assets.
"""
import math
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(r"C:\Users\MAXIMO\Desktop\Astraeon\graphics\characters\main_player")
SCENE = "CHR_Astraeon_Player_Work"
COLORS = {
    "fabric": (0.025, 0.045, 0.075, 1),
    "white": (0.73, 0.77, 0.80, 1),
    "skin": (0.53, 0.31, 0.21, 1),
    "hair": (0.045, 0.024, 0.013, 1),
    "cyan": (0.02, 0.5, 0.7, 1),
}


def collection(parent, name):
    col = bpy.data.collections.new(name)
    parent.children.link(col)
    return col


def surface(col, name, vertices, faces, color):
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    col.objects.link(obj)
    obj.color = COLORS[color]
    obj["AstraeonStage"] = "PROXY_NOT_GAME_READY"
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    return obj


def loft(col, name, stations, color, sides=16, axis=None):
    """Stations: center, X radius, Y radius; plane perpendicular to centerline."""
    axis = Vector(axis).normalized() if axis is not None else Vector((0, 0, 1))
    front = Vector((0, -1, 0))
    right = axis.cross(front).normalized()
    front = right.cross(axis).normalized()
    verts = []
    for center, rx, ry in stations:
        c = Vector(center)
        for j in range(sides):
            t = j * math.tau / sides
            verts.append(tuple(c + right * (rx * math.cos(t)) + front * (ry * math.sin(t))))
    faces = []
    for row in range(len(stations) - 1):
        for j in range(sides):
            k = (j + 1) % sides
            faces.append((row*sides+j, row*sides+k, (row+1)*sides+k, (row+1)*sides+j))
    faces.extend([tuple(reversed(range(sides))), tuple((len(stations)-1)*sides+j for j in range(sides))])
    return surface(col, name, verts, [tuple(reversed(face)) for face in faces], color)


def segment(col, name, start, end, radius, color):
    a, b = Vector(start), Vector(end)
    return loft(col, name, [(a, radius*.85, radius*.78), (a.lerp(b,.15),radius,radius*.92),
                           (a.lerp(b,.6),radius*.86,radius*.8), (b,radius*.65,radius*.6)], color, axis=b-a)


def camera(scene, col, name, pos, target):
    data = bpy.data.cameras.new(name + "_Data")
    obj = bpy.data.objects.new(name, data)
    col.objects.link(obj)
    obj.location = pos
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z', 'Y').to_euler()
    data.type = 'ORTHO'
    data.ortho_scale = 2.15
    data.clip_start = .01
    return obj


def build_primary():
    if bpy.data.scenes.get(SCENE):
        raise RuntimeError('Proxy scene already exists; inspect and edit it instead of duplicating.')
    scene = bpy.data.scenes.new(SCENE)
    bpy.context.window.scene = scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1
    scene.render.fps = 30
    scene.frame_start, scene.frame_end = 1, 61
    root = collection(scene.collection, 'ASTRAEON_CHARACTER')
    geo = collection(root, 'ASTRAEON_GEO')
    body = collection(geo, 'ASTRAEON_BODY')
    for name in ['SUIT','HELMET','GEAR']:
        collection(geo, 'ASTRAEON_' + name)
    for name in ['RIG','ANIMATIONS','ATTACHMENTS','COLLISION_HELPERS','EXPORT','TEST']:
        collection(root, 'ASTRAEON_' + name)
    studio = collection(root, 'ASTRAEON_QA_STUDIO')
    loft(body,'PROXY_Player_Torso',[
        ((0,0,.90),.13,.085),((0,0,1.00),.158,.10),((0,0,1.11),.125,.09),
        ((0,0,1.22),.15,.105),((0,0,1.36),.19,.13),((0,0,1.46),.203,.12),
        ((0,0,1.52),.14,.09)],'fabric',24)
    loft(body,'PROXY_Player_Neck', [((0,0,1.50),.064,.06),((0,0,1.63),.061,.058)],'fabric')
    loft(body,'PROXY_Player_Head',[
        ((0,-.015,1.59),.039,.04),((0,-.023,1.62),.06,.071),
        ((0,-.013,1.67),.077,.089),((0,0,1.72),.078,.095),
        ((0,.005,1.78),.074,.091),((0,.007,1.82),.043,.055),
        ((0,.007,1.83),.003,.004)],'skin',24)
    for side, sign in [('l',1),('r',-1)]:
        p = lambda x,y,z: (x*sign,y,z)
        segment(body,'PROXY_Player_UpperArm_'+side,p(.212,0,1.47),p(.36,0,1.20),.071,'fabric')
        segment(body,'PROXY_Player_Forearm_'+side,p(.36,0,1.20),p(.477,-.015,.977),.057,'fabric')
        segment(body,'PROXY_Player_Palm_'+side,p(.477,-.015,.977),p(.520,-.016,.889),.041,'fabric')
        for finger, dx, dz in [('index',-.024,0),('middle',-.009,-.008),('ring',.007,-.004),('pinky',.021,.008)]:
            segment(body,'PROXY_Player_'+finger+'_'+side,p(.518+dx,-.016,.902+dz),p(.553+dx,-.018,.830+dz),.008,'fabric')
        segment(body,'PROXY_Player_thumb_'+side,p(.479,-.043,.94),p(.468,-.06,.876),.010,'fabric')
        loft(body,'PROXY_Player_Leg_'+side,[(p(.095,0,.98),.092,.091),
             (p(.105,0,.83),.091,.092),(p(.106,-.022,.55),.059,.065),
             (p(.107,-.017,.50),.057,.061),(p(.107,.005,.37),.065,.066),
             (p(.107,0,.14),.043,.048)],'fabric')
        loft(body,'PROXY_Player_Boot_'+side,[(p(.107,-.057,0),.065,.139),
             (p(.107,-.057,.035),.065,.139),(p(.107,-.05,.086),.062,.13),
             (p(.107,-.005,.145),.05,.067),(p(.107,0,.205),.051,.059)],'white')
    scene.camera = camera(scene,studio,'CAM_Player_Front',(0,-4,.96),(0,0,.96))
    camera(scene,studio,'CAM_Player_Side',(4,0,.96),(0,0,.96))
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.render.resolution_x, scene.render.resolution_y = 900,1100
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'OBJECT'
    scene.display.shading.show_shadows = True
    scene.display.shading.show_cavity = True
    scene.display.shading.background_type = 'WORLD'
    scene.world = bpy.data.worlds.new('WORLD_Player_QA')
    scene.world.color = (.13,.13,.13)
    scene.view_settings.view_transform = 'Standard'
    scene['ProductionStatus'] = 'PROXY_ONLY_NOT_GAME_READY'
    for area in bpy.context.screen.areas:
        if area.type == 'VIEW_3D':
            area.spaces.active.region_3d.view_perspective = 'CAMERA'
            area.spaces.active.shading.color_type = 'OBJECT'
    return scene


def render_views(suffix=''):
    scene = bpy.data.scenes[SCENE]
    out = ROOT/'references'
    out.mkdir(exist_ok=True,parents=True)
    paths=[]
    for view in ['Front','Side']:
        scene.camera = bpy.data.objects['CAM_Player_'+view]
        target = out/('PROXY_Proportions_'+view+suffix+'.png')
        if target.exists():
            raise RuntimeError('Preview exists; preserve the previous review and choose a new suffix.')
        scene.render.filepath = str(target)
        bpy.ops.render.render(write_still=True)
        paths.append(str(target))
    scene.camera = bpy.data.objects['CAM_Player_Front']
    return paths
