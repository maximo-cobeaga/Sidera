"""Exportacion del personaje principal a FBX para Unreal Engine 5.7.4.

Respeta el contrato del exportador existente del proyecto
(`Tools/Blender/exporters/fbx.py`): escala global 1, unidades aplicadas,
Forward -Y / Up Z, sin leaf bones y sin aplicar modificadores.

Se exporta por separado:
  - SK_Astraeon_Player.fbx        malla + esqueleto, sin animacion
  - SK_Astraeon_Player_LODn.fbx   niveles de detalle con el mismo esqueleto
  - AN_Astraeon_Player_All.fbx    una pista por Action (45 clips)

Las texturas se escriben como PNG sueltos; el material PBR se reconstruye en
Unreal. Ver MAIN_CHARACTER_PIPELINE.md, seccion Unreal Import.
"""
from pathlib import Path
import bpy

RIG_NAME = 'SKEL_Astraeon_Player'
MESH_NAME = 'SK_Astraeon_Player'
# Unreal descarta el contenedor de armature cuando se llama 'Armature'; asi el
# hueso `root` queda como raiz real del Skeleton y el root motion funciona.
EXPORT_ARMATURE_NAME = 'Armature'

FBX_COMMON = dict(
    use_selection=True, global_scale=1.0, apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_NONE', axis_forward='-Y', axis_up='Z',
    add_leaf_bones=False, use_mesh_modifiers=False, mesh_smooth_type='FACE',
    primary_bone_axis='Y', secondary_bone_axis='X', path_mode='AUTO',
    object_types={'ARMATURE', 'MESH'},
)


def save_textures(dest):
    """Guarda el set horneado conservando el nombre de cada mapa.

    La version anterior derivaba el nombre de salida del primer token del
    datablock (`img.name.split('_')[0]`). Con el set separado Character/Suit ese
    token es siempre 'T', asi que los ocho mapas se escribian sobre el mismo
    archivo y ademas quedaban repuntados a el. Se exporta por nombre propio y
    solo lo que pertenece al personaje.
    """
    dest = Path(dest)
    dest.mkdir(parents=True, exist_ok=True)
    written = []
    for img in bpy.data.images:
        if img.type != 'IMAGE' or not img.has_data:
            continue
        if not img.name.startswith('T_Player_'):
            continue
        out = dest / (img.name + '.png')
        img.file_format = 'PNG'
        img.filepath_raw = str(out)
        img.save()
        written.append({'image': img.name, 'file': str(out),
                        'size': list(img.size),
                        'colorspace': img.colorspace_settings.name})
    return written


def reset_pose(rig):
    """Devuelve el rig a la pose de reposo.

    Quitar la Action no restaura los pose bones: conservan el ultimo valor
    evaluado. Sin esto el FBX de malla se exporta posado y Unreal toma esa
    pose como reference pose del Skeleton.
    """
    for pb in rig.pose.bones:
        pb.location = (0.0, 0.0, 0.0)
        pb.rotation_euler = (0.0, 0.0, 0.0)
        pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        pb.scale = (1.0, 1.0, 1.0)
    bpy.context.view_layer.update()


def prepare_grip_export_samples():
    """UE rejects zero-duration takes. Keep grips static over two samples.

    Logical pose duration stays zero in the contract; exported duration is one
    30 Hz interval. Idempotent and compatible with Blender's slotted actions.
    """
    changed = []
    for action in bpy.data.actions:
        if not action.name.startswith('Grip_'):
            continue
        for layer in action.layers:
            for strip in layer.strips:
                for bag in strip.channelbags:
                    for curve in bag.fcurves:
                        value = curve.evaluate(1)
                        point = next((k for k in curve.keyframe_points if abs(k.co.x - 2) < .001), None)
                        if point is None:
                            point = curve.keyframe_points.insert(2, value)
                        point.co.y = value
                        point.interpolation = 'CONSTANT'
                        curve.update()
        action.use_frame_range = True
        action.frame_start = 1
        action.frame_end = 2
        changed.append(action.name)
    return changed


def export_animations(root):
    root = Path(root)
    rig = bpy.data.objects[RIG_NAME]
    old_name = rig.name
    old_action = rig.animation_data.action if rig.animation_data else None
    prepare_grip_export_samples()
    rig.name = EXPORT_ARMATURE_NAME
    try:
        return _export(root / 'exports/AN_Astraeon_Player_All.fbx', [rig],
                       object_types={'ARMATURE'}, bake_anim=True,
                       bake_anim_use_all_bones=True, bake_anim_use_nla_strips=False,
                       bake_anim_use_all_actions=True, bake_anim_force_startend_keying=True,
                       bake_anim_step=1.0, bake_anim_simplify_factor=0.0)
    finally:
        rig.name = old_name
        rig.animation_data.action = old_action
        reset_pose(rig)


def _select(objs):
    bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.hide_viewport = False
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]


def _export(path, objs, **extra):
    _select(objs)
    kwargs = dict(FBX_COMMON)
    kwargs.update(extra)
    res = bpy.ops.export_scene.fbx(filepath=str(path), **kwargs)
    p = Path(path)
    if 'FINISHED' not in res or not p.is_file() or p.stat().st_size == 0:
        raise RuntimeError('Fallo la exportacion FBX: ' + str(path))
    return {'file': str(p), 'bytes': p.stat().st_size}


def export_all(root=None):
    root = Path(root or r'C:\Users\MAXIMO\Desktop\Astraeon\graphics\characters\main_player')
    exports = root / 'exports'
    exports.mkdir(parents=True, exist_ok=True)
    prepare_grip_export_samples()

    scene = bpy.context.scene
    scene.render.fps = 30
    scene.render.fps_base = 1.0

    rig = bpy.data.objects[RIG_NAME]
    mesh = bpy.data.objects[MESH_NAME]
    lods = [bpy.data.objects['%s_LOD%d' % (MESH_NAME, n)] for n in (1, 2, 3)]
    helmet = bpy.data.objects.get('SK_Astraeon_Helmet')

    out = {'textures': save_textures(root / 'textures'), 'fbx': []}

    original = rig.name
    rig.name = EXPORT_ARMATURE_NAME
    prev_action = rig.animation_data.action if rig.animation_data else None
    if rig.animation_data:
        rig.animation_data.action = None
    reset_pose(rig)
    try:
        out['fbx'].append(dict(kind='skeletal_mesh',
                               **_export(exports / (MESH_NAME + '.fbx'), [rig, mesh],
                                         bake_anim=False)))
        for n, lod in zip((1, 2, 3), lods):
            out['fbx'].append(dict(kind='lod%d' % n,
                                   **_export(exports / ('%s_LOD%d.fbx' % (MESH_NAME, n)),
                                             [rig, lod], bake_anim=False)))
        for kind, name in (('helmet', 'SK_Astraeon_Helmet'),
                           ('backpack', 'SK_Astraeon_Backpack'),
                           ('wrist_computer', 'SK_Astraeon_WristComputer')):
            piece = bpy.data.objects.get(name)
            if piece is None:
                continue
            out['fbx'].append(dict(kind=kind,
                                   **_export(exports / (name + '.fbx'),
                                             [rig, piece], bake_anim=False,
                                             use_mesh_modifiers=True)))
        out['fbx'].append(dict(kind='animations',
                               **_export(exports / 'AN_Astraeon_Player_All.fbx', [rig],
                                         object_types={'ARMATURE'},
                                         bake_anim=True,
                                         bake_anim_use_all_bones=True,
                                         bake_anim_use_nla_strips=False,
                                         bake_anim_use_all_actions=True,
                                         bake_anim_force_startend_keying=True,
                                         bake_anim_step=1.0,
                                         bake_anim_simplify_factor=0.0)))
    finally:
        rig.name = original
        if rig.animation_data:
            rig.animation_data.action = prev_action
    return out


if __name__ == '__main__':
    export_all()
