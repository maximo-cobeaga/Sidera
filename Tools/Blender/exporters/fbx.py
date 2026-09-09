from pathlib import Path
import bpy


def export_asset(obj, directory):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = directory / (obj.name + '.fbx')
    result = bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
        object_types={'MESH'}, global_scale=1.0, apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_NONE', axis_forward='-Y', axis_up='Z',
        bake_anim=False, add_leaf_bones=False, use_mesh_modifiers=False,
        mesh_smooth_type='FACE', path_mode='AUTO')
    if 'FINISHED' not in result or not path.is_file() or path.stat().st_size == 0:
        raise RuntimeError('FBX export failed: ' + str(path))
    return path
