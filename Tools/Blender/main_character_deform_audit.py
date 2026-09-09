"""Auditoria visual de deformacion del protagonista.

`docs/skin_polish.json` dejaba `deformation_visual_audit: pending`. Aqui se
flexionan las articulaciones criticas a angulos de trabajo y se renderiza cada
una, para poder ver si el skinning colapsa el volumen o rompe la silueta.

No se anima nada permanente: las poses se aplican, se renderizan y se deshacen.
"""
import json
import math
from pathlib import Path
import bpy
from mathutils import Euler

ROOT = Path(r'C:/Users/MAXIMO/Desktop/Astraeon')
ASSET = ROOT / 'graphics/characters/main_player'
REFS = ASSET / 'references'
RIG = 'SKEL_Astraeon_Player'
BODY = 'SK_Astraeon_Player'

# (nombre, {hueso: (rx, ry, rz) en grados}, camara, ortho_scale, objetivo)
POSES = [
    ('Elbow', {'lowerarm_l': (0, 0, -95)}, 'CAM_Player_Front', 0.75, 'lowerarm_l'),
    ('Shoulder', {'upperarm_l': (0, 0, -75)}, 'CAM_Player_Front', 0.95, 'upperarm_l'),
    ('Hip', {'thigh_l': (0, 0, 85)}, 'CAM_Player_Side', 1.10, 'thigh_l'),
    ('Knee', {'calf_l': (0, 0, -105)}, 'CAM_Player_Side', 0.90, 'calf_l'),
]


def _volume(obj, depsgraph):
    """Volumen aproximado de la malla evaluada, para detectar colapsos."""
    evaluated = obj.evaluated_get(depsgraph)
    mesh = evaluated.to_mesh()
    mesh.calc_loop_triangles()
    total = 0.0
    for tri in mesh.loop_triangles:
        a, b, c = (mesh.vertices[i].co for i in tri.vertices)
        total += a.dot(b.cross(c)) / 6.0
    evaluated.to_mesh_clear()
    return abs(total)


def audit(render=True):
    scene = bpy.context.scene
    rig = bpy.data.objects[RIG]
    body = bpy.data.objects[BODY]
    previous_camera = scene.camera
    previous_action = rig.animation_data.action if rig.animation_data else None
    if rig.animation_data:
        rig.animation_data.action = None
    for bone in rig.pose.bones:
        bone.matrix_basis.identity()
    bpy.context.view_layer.update()

    depsgraph = bpy.context.evaluated_depsgraph_get()
    rest_volume = _volume(body, depsgraph)
    report = {'status': 'DEFORMATION_AUDITED', 'rest_volume_l': round(rest_volume * 1000, 3),
              'poses': []}

    for name, bones, camera, scale, focus in POSES:
        for bone_name, angles in bones.items():
            bone = rig.pose.bones[bone_name]
            bone.rotation_mode = 'XYZ'
            bone.rotation_euler = Euler([math.radians(a) for a in angles], 'XYZ')
        bpy.context.view_layer.update()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        volume = _volume(body, depsgraph)

        entry = {'pose': name, 'bones': {k: list(v) for k, v in bones.items()},
                 'volume_l': round(volume * 1000, 3),
                 'volume_change_pct': round(100 * (volume - rest_volume) / rest_volume, 2)}

        if render:
            cam = bpy.data.objects[camera]
            head = rig.matrix_world @ rig.pose.bones[focus].head
            cam.data.ortho_scale = scale
            offset = cam.location - cam.matrix_world.translation
            location = cam.location.copy()
            # encuadre sobre la articulacion, conservando el eje de la camara
            if camera.endswith('Front'):
                cam.location = (head.x, location.y, head.z)
            else:
                cam.location = (location.x, head.y, head.z)
            scene.camera = cam
            path = REFS / ('QA_Deform_%s.png' % name)
            scene.render.filepath = str(path)
            bpy.ops.render.render(write_still=True)
            cam.location = location
            entry['render'] = str(path)

        report['poses'].append(entry)
        for bone_name in bones:
            rig.pose.bones[bone_name].matrix_basis.identity()
        bpy.context.view_layer.update()

    if rig.animation_data:
        rig.animation_data.action = previous_action
    scene.camera = previous_camera
    bpy.context.view_layer.update()
    (ASSET / 'docs/deformation_audit.json').write_text(json.dumps(report, indent=2) + '\n')
    return report
