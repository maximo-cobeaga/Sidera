"""Inspect a saved file in an isolated Blender process; never save the scene.

This is not a Bridge connectivity test and cannot inspect another process's scene.
Run with --background <file.blend> --disable-autoexec --python-exit-code 1
--python Tools/Blender/inspect_main_character_preflight.py -- --report <new.json>.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import bpy


def inspect():
    scene = bpy.context.scene
    return {
        'blender_version': bpy.app.version_string,
        'background': bpy.app.background,
        'filepath': bpy.data.filepath,
        'units': {'system': scene.unit_settings.system,
                  'scale_length': scene.unit_settings.scale_length},
        'fps': scene.render.fps / scene.render.fps_base,
        'frame': scene.frame_current,
        'collections': [c.name for c in bpy.data.collections],
        'objects': [
            {'name': o.name, 'type': o.type, 'location': list(o.location),
             'rotation': list(o.rotation_euler), 'scale': list(o.scale),
             'dimensions': list(o.dimensions),
             'collections': [c.name for c in o.users_collection],
             'bones': [{'name': b.name, 'parent': b.parent.name if b.parent else None}
                       for b in o.data.bones] if o.type == 'ARMATURE' else [],
             'materials': [s.name for s in o.material_slots]}
            for o in scene.objects],
        'actions': [{'name': a.name, 'frame_range': list(a.frame_range)}
                    for a in bpy.data.actions],
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--report', required=True)
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
    target = Path(args.report).resolve()
    if target.exists():
        raise FileExistsError(f'Refusing to replace report: {target}')
    if not bpy.app.background:
        raise RuntimeError('This preflight is restricted to an isolated background process')
    source = Path(bpy.data.filepath)
    before_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    before = inspect()
    probe_mesh = bpy.data.meshes.new('TEST_Astraeon_PreflightMesh')
    probe = bpy.data.objects.new('TEST_Astraeon_Preflight', probe_mesh)
    action = None
    try:
        bpy.context.scene.collection.objects.link(probe)
        probe_mesh.from_pydata([(0, 0, 0), (1, 0, 0), (0, 1, 0)], [], [(0, 1, 2)])
        probe_mesh.vertices[0].co.z = 0.25
        probe.location.x = 0.125
        probe.keyframe_insert(data_path='location', frame=1)
        action = probe.animation_data.action
        probe.location.x = 0.5
        probe.keyframe_insert(data_path='location', frame=31)
        assert len(probe_mesh.polygons) == 1
        assert abs(probe_mesh.vertices[0].co.z - 0.25) < 1e-6
        assert tuple(action.frame_range) == (1.0, 31.0)
    finally:
        bpy.data.objects.remove(probe, do_unlink=True)
        bpy.data.meshes.remove(probe_mesh)
        if action is not None:
            bpy.data.actions.remove(action)
    assert inspect() == before, 'Scene inventory changed after temporary probe cleanup'
    assert hashlib.sha256(source.read_bytes()).hexdigest() == before_hash
    report = {
        'scope': 'isolated local Blender; saved legacy file only',
        'bridge_local_connection': 'NOT VERIFIED',
        'active_user_scene': 'NOT VERIFIED',
        'character_animation_service': 'NOT VERIFIED',
        'local_mesh_create_modify_cleanup': 'PASS',
        'local_object_keyframe_creation': 'PASS',
        'source_file_unchanged_sha256': before_hash,
        'scene': before,
        'new_character_qa': 'NOT VERIFIED',
    }
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open('x', encoding='utf-8') as handle:
        json.dump(report, handle, indent=2)
    print(json.dumps({'report': str(target), 'version': before['blender_version'],
                      'objects': len(before['objects']), 'actions': len(before['actions']),
                      'local_probe': 'PASS', 'bridge': 'NOT VERIFIED'}))


if __name__ == '__main__':
    main()
