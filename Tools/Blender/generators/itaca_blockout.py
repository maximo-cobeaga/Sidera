"""Run with Blender --background --python-exit-code 1 --python <this file>."""
import hashlib
import json
from pathlib import Path
import sys

import bpy
from mathutils import Vector

TOOL_ROOT = Path(__file__).resolve().parents[1]
ROOT = TOOL_ROOT.parents[1]
sys.path.insert(0, str(TOOL_ROOT))
from common.scene import reset_scene, create_asset, signature
from validators.mesh import validate
from exporters.fbx import export_asset
from tests.checks import run as run_tests


def preview(objects, output):
    # Work on presentation duplicates; source assets retain applied transforms and base pivots.
    positions = [(0,-0.2,0), (-2,0,0), (-2,1,0.12), (1.8,1,0), (1.6,-0.6,0)]
    for obj, position in zip(objects, positions):
        obj.location = position
    scene = bpy.context.scene
    bpy.ops.object.camera_add(location=(6,-10,6))
    camera = bpy.context.object
    camera.rotation_euler = (Vector((0,0,1.1)) - camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = 8.5
    scene.camera = camera
    for position, energy, size in [((1,-4,7),1600,6), ((-4,2,5),1000,5)]:
        bpy.ops.object.light_add(type='AREA', location=position)
        lamp = bpy.context.object
        lamp.data.energy, lamp.data.shape, lamp.data.size = energy, 'DISK', size
        lamp.rotation_euler = (Vector((0,0,1)) - lamp.location).to_track_quat('-Z','Y').to_euler()
    scene.world.color = (0.12,0.12,0.12)
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 16
    scene.cycles.device = 'CPU'
    scene.render.resolution_x, scene.render.resolution_y = 1100, 750
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.filepath = str(output / 'Itaca_Blockout_Preview.png')
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'Itaca_Blockout_Preview.blend'))
    bpy.ops.render.render(write_still=True)


def main():
    config_path = TOOL_ROOT / 'configs/itaca_blockout.json'
    config = json.loads(config_path.read_text(encoding='utf-8'))
    output = ROOT / 'ContentPipeline/Generated/ItacaBlockout'
    reports = ROOT / 'ContentPipeline/reports'
    output.mkdir(parents=True, exist_ok=True)
    reports.mkdir(parents=True, exist_ok=True)
    report_path = reports / 'itaca_blockout_validation.json'
    report = {'schema_version':1, 'passed':False, 'stage':'started',
              'blender_version':bpy.app.version_string, 'generator_version':config['generator_version'],
              'seed':config['seed'], 'unreal_integration':'not_checked_here',
              'config_sha256':hashlib.sha256(config_path.read_bytes()).hexdigest()}
    try:
        reset_scene()
        tests = run_tests(config)
        objects = [create_asset(spec) for spec in config['assets']]
        bpy.context.view_layer.update()
        results = [validate(obj, spec) for obj, spec in zip(objects, config['assets'])]
        report.update(tests=tests, assets=results)
        if not all(r['passed'] for r in tests + results):
            raise RuntimeError('Source validation or negative controls failed')
        if len({o.name for o in objects}) != len(config['assets']):
            raise RuntimeError('Duplicate source names')
        for obj, result in zip(objects, results):
            result['geometry_sha256'] = signature(obj)
            result['fbx'] = export_asset(obj, output).relative_to(ROOT).as_posix()
            result['fbx_sha256'] = hashlib.sha256((ROOT / result['fbx']).read_bytes()).hexdigest()
        bpy.ops.wm.save_as_mainfile(filepath=str(output / 'Itaca_Blockout_Source.blend'))
        # Validate the transport as well as the source; one mesh per FBX, correct metric bounds.
        for spec, result in zip(config['assets'], results):
            reset_scene()
            bpy.ops.import_scene.fbx(filepath=str(ROOT / result['fbx']))
            meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
            bpy.context.view_layer.update()
            ok = len(meshes) == 1 and all(abs(meshes[0].dimensions[i] - spec['dimensions_m'][i]) < 1e-4 for i in range(3))
            result['fbx_roundtrip_passed'] = ok
            if not ok:
                raise RuntimeError('FBX object count/scale roundtrip failed: ' + spec['id'])
        reset_scene()
        objects = [create_asset(spec) for spec in config['assets']]
        preview(objects, output)
        report.update(passed=True, stage='source_export_roundtrip_preview_complete')
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('ASTRAEON_ART_VALIDATION: PASS ' + str(report_path))


if __name__ == '__main__':
    main()
