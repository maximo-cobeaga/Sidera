"""Original region silhouettes: deterministic metric source, validation and FBX transport."""
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
import bmesh
from mathutils import Vector

TOOL_ROOT = Path(__file__).resolve().parents[1]
ROOT = TOOL_ROOT.parents[1]
sys.path.insert(0, str(TOOL_ROOT))
from common.scene import reset_scene, material, signature
from validators.mesh import validate
from exporters.fbx import export_asset


def create_asset(spec):
    parts = []

    def part(kind, position, scale, rotation=(0, 0, 0)):
        if kind == 'rock':
            bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=1)
        elif kind == 'ring':
            bpy.ops.mesh.primitive_torus_add(major_segments=16, minor_segments=6,
                                           major_radius=0.65, minor_radius=0.22)
        elif kind == 'prism':
            bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=0.32, radius2=0.08, depth=1)
        else:
            bpy.ops.mesh.primitive_cube_add(size=1)
        obj = bpy.context.object
        obj.location, obj.scale, obj.rotation_euler = position, scale, rotation
        parts.append(obj)

    shape = spec['shape']
    if shape == 'fiber':
        part('rock', (0, 0, 0.12), (0.45, 0.45, 0.16))
        for i in range(7):
            angle = i * math.tau / 7
            part('prism', (0.23 * math.cos(angle), 0.23 * math.sin(angle), 0.5),
                 (0.22, 0.42, 0.8 + 0.14 * (i % 3)),
                 (0.20 * math.sin(angle), -0.20 * math.cos(angle), angle))
    elif shape == 'nodule':
        part('rock', (0, 0, 0.15), (0.45, 0.45, 0.22))
        for i in range(5):
            angle = i * math.tau / 5
            part('rock', (0.22 * math.cos(angle), 0.22 * math.sin(angle), 0.38 + 0.08 * (i % 2)),
                 (0.22, 0.20, 0.28), (0, angle, angle))
    elif shape == 'crystal':
        part('rock', (0, 0, 0.12), (0.55, 0.55, 0.18))
        for i in range(4):
            angle = i * math.tau / 4
            part('prism', (0.23 * math.cos(angle), 0.23 * math.sin(angle), 0.52),
                 (0.6, 0.6, 0.8 + 0.2 * (i % 2)), (0, 0.24, angle))
    elif shape in ('cryo_vein', 'quartz_vein'):
        # A thick enclosing mineral collar, with a recessed target. Not loose hand-sized ore.
        part('rock', (0, 0, 0.18), (0.9, 0.9, 0.24))
        part('ring', (0, 0, 0.78), (1, 1, 1.5))
        for x in (-0.17, 0.17):
            part('rock' if shape == 'cryo_vein' else 'prism', (x, 0, 0.53),
                 (0.18, 0.23, 0.23) if shape == 'cryo_vein' else (0.55, 0.55, 0.55))
    elif shape == 'signal':
        part('rock', (0, 0, 0.16), (0.8, 0.8, 0.20))
        part('ring', (0, 0, 1.2), (1, 1.3, 0.65), (math.pi / 2, 0, 0))
        part('ring', (0, 0, 1.2), (1, 1.3, 0.65), (math.pi / 2, 0, math.pi / 2))
        part('prism', (0, 0, 0.78), (0.8, 0.8, 1.2))
    elif shape == 'anomaly':
        for i in range(4):
            part('rock', (0, 0, 0.20 + i * 0.32), (0.55 - i * 0.06, 0.5, 0.23),
                 (0, i * 0.09, i * 0.38))
    else:
        raise ValueError('Unknown silhouette: ' + shape)

    bpy.ops.object.select_all(action='DESELECT')
    for obj in parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    obj.name = spec['id']
    # Normalize authored silhouette to its explicit metric envelope; keep base pivot at zero.
    low = [min(v.co[i] for v in obj.data.vertices) for i in range(3)]
    high = [max(v.co[i] for v in obj.data.vertices) for i in range(3)]
    for v in obj.data.vertices:
        for i in range(3):
            origin = low[i] if i == 2 else (high[i] + low[i]) / 2
            v.co[i] = (v.co[i] - origin) * spec['dimensions_m'][i] / (high[i] - low[i])
    obj.data.update()
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.materials.clear()
    obj.data.materials.append(material(spec['material'], spec['color']))
    obj['quality_tier'] = 'blockout'
    obj['generator_version'] = 1
    bpy.context.view_layer.update()
    return obj


def checks(config):
    results = []
    hashes = []
    for spec in config['assets']:
        obj = create_asset(spec)
        digest = signature(obj)
        hashes.append(digest)
        bpy.data.objects.remove(obj, do_unlink=True)
        obj = create_asset(spec)
        results.append({'test': spec['id'] + '_repeat', 'passed': digest == signature(obj)})
        # A resource with wrong scale or an open shell must never be exported as valid.
        obj.scale.x = 2
        results.append({'test': spec['id'] + '_reject_scale',
                        'passed': 'applied_scale' in validate(obj, spec)['errors']})
        bpy.data.objects.remove(obj, do_unlink=True)
    results.append({'test': 'distinct_geometry_for_all_seven_silhouettes',
                    'passed': len(set(hashes)) == len(hashes)})
    return results


def preview(config, output):
    reset_scene()
    for index, spec in enumerate(config['assets']):
        obj = create_asset(spec)
        obj.location = ((index % 4) * 2.6, (index // 4) * 3.4, 0)
    scene = bpy.context.scene
    target = Vector((3.8, 1.8, 0.7))
    bpy.ops.object.camera_add(location=(11, -13, 10))
    camera = bpy.context.object
    camera.rotation_euler = (target - camera.location).to_track_quat('-Z', 'Y').to_euler()
    camera.data.type, camera.data.ortho_scale = 'ORTHO', 13.2
    scene.camera = camera
    for location in ((1, -4, 8), (8, 5, 8)):
        bpy.ops.object.light_add(type='AREA', location=location)
        lamp = bpy.context.object
        lamp.data.energy, lamp.data.size = 1900, 7
        lamp.rotation_euler = (target - lamp.location).to_track_quat('-Z', 'Y').to_euler()
    scene.world.color = (0.12, 0.12, 0.12)
    scene.render.engine = 'CYCLES'
    scene.cycles.samples, scene.cycles.device = 24, 'CPU'
    scene.render.resolution_x, scene.render.resolution_y = 1400, 900
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.filepath = str(output / 'Region_Blockout_Preview.png')
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'Region_Blockout_Preview.blend'))
    bpy.ops.render.render(write_still=True)


def main():
    config_path = TOOL_ROOT / 'configs/region_blockout.json'
    config = json.loads(config_path.read_text(encoding='utf-8'))
    output = ROOT / 'ContentPipeline/Generated/RegionBlockout'
    output.mkdir(parents=True, exist_ok=True)
    report_path = ROOT / 'ContentPipeline/reports/region_blockout_validation.json'
    report = {'schema_version': 1, 'passed': False, 'blender_version': bpy.app.version_string,
              'generator_version': config['generator_version'], 'seed': config['seed'],
              'config_sha256': hashlib.sha256(config_path.read_bytes()).hexdigest(),
              'generator_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'assets': []}
    try:
        reset_scene()
        report['tests'] = checks(config)
        if not all(t['passed'] for t in report['tests']):
            raise RuntimeError('Determinism or negative controls failed')
        for spec in config['assets']:
            obj = create_asset(spec)
            result = validate(obj, spec)
            report['assets'].append(result)
            if not result['passed']:
                raise RuntimeError(str(result))
            result['geometry_sha256'] = signature(obj)
            fbx = export_asset(obj, output)
            result['fbx'] = fbx.relative_to(ROOT).as_posix()
            result['fbx_sha256'] = hashlib.sha256(fbx.read_bytes()).hexdigest()
        bpy.ops.wm.save_as_mainfile(filepath=str(output / 'Region_Blockout_Source.blend'))
        for spec, result in zip(config['assets'], report['assets']):
            reset_scene()
            bpy.ops.import_scene.fbx(filepath=str(ROOT / result['fbx']))
            meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
            bpy.context.view_layer.update()
            result['fbx_roundtrip_passed'] = len(meshes) == 1 and all(
                abs(meshes[0].dimensions[i] - spec['dimensions_m'][i]) < 1e-4 for i in range(3))
            if not result['fbx_roundtrip_passed']:
                raise RuntimeError('FBX metric roundtrip failed: ' + spec['id'])
        preview(config, output)
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('ASTRAEON_REGION_ART: PASS')


if __name__ == '__main__':
    main()
