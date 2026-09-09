"""Itaca exterior modules and interior stations.

Same declarative kit as itaca_blockout.py, and it reuses that family's certified static
contract (validators/mesh.py, tests/checks.py, exporters/fbx.py) without touching it: the
sha256 of the itaca generator is an input to an approved report, so it stays frozen and this
runner lives beside it instead of extending it.

Run with Blender --background --factory-startup --python-exit-code 1 --python <this file>.
"""
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

CONFIG_PATH = TOOL_ROOT / 'configs/ship_blockout.json'
OUT = ROOT / 'ContentPipeline/Generated/ShipBlockout'
REPORT = ROOT / 'ContentPipeline/reports/ship_blockout_validation.json'


def spec_by_id(config, identifier):
    return next(spec for spec in config['assets'] if spec['id'] == identifier)


def place(config, identifier, location, rotation_z=0.0):
    obj = create_asset(spec_by_id(config, identifier))
    obj.location = location
    obj.rotation_euler = (0.0, 0.0, rotation_z)
    return obj


def studio(scene, camera_location, target, ortho_scale, resolution):
    bpy.ops.object.camera_add(location=camera_location)
    camera = bpy.context.object
    camera.rotation_euler = (Vector(target) - camera.location).to_track_quat('-Z', 'Y').to_euler()
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = ortho_scale
    scene.camera = camera
    for position, energy, size in [((6, -14, 12), 4000, 10), ((-10, 6, 9), 2400, 9)]:
        bpy.ops.object.light_add(type='AREA', location=position)
        lamp = bpy.context.object
        lamp.data.energy, lamp.data.shape, lamp.data.size = energy, 'DISK', size
        lamp.rotation_euler = (Vector(target) - lamp.location).to_track_quat('-Z', 'Y').to_euler()
    scene.world.color = (0.12, 0.12, 0.12)
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 16
    scene.cycles.device = 'CPU'
    scene.render.resolution_x, scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'


def preview_exterior(config):
    """Mount the modules exactly as the pawn assembles them, so the study volume is real."""
    reset_scene()
    mount = config['assembly']
    base = mount['hull_mount_z_m']
    place(config, 'SM_Itaca_Hull_Blockout', (0, 0, base))
    for side in (-1, 1):
        place(config, 'SM_Itaca_Engine_Blockout',
              (mount['engine_mount_x_m'], side * mount['engine_mount_y_m'], base + mount['engine_mount_z_m']))
        for x in mount['gear_mount_x_m']:
            place(config, 'SM_Itaca_LandingGear_Blockout', (x, side * mount['gear_mount_y_m'], 0))
    place(config, 'SM_Itaca_Antenna_Blockout', (mount['antenna_mount_x_m'], 0, base + mount['antenna_mount_z_m']))

    bpy.ops.mesh.primitive_plane_add(size=80, location=(0, 0, -0.02))
    floor = bpy.context.object
    floor.name = 'Preview_Floor_DoNotExport'
    scene = bpy.context.scene
    studio(scene, (15, -17, 5.2), (0, 0, 1.9), 20.0, (1300, 820))
    scene.render.filepath = str(OUT / 'Itaca_Exterior_Preview.png')
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'Itaca_Exterior_Preview.blend'))
    bpy.ops.render.render(write_still=True)


def preview_interior(config):
    reset_scene()
    place(config, 'SM_Itaca_PilotConsole_Blockout', (-1.4, 0, 0))
    place(config, 'SM_Itaca_Fabricator_Blockout', (0.2, 0, 0))
    place(config, 'SM_Itaca_HatchLeaf_Blockout', (1.9, 0, 0))
    bpy.ops.mesh.primitive_plane_add(size=20, location=(0, 0, -0.02))
    floor = bpy.context.object
    floor.name = 'Preview_Floor_DoNotExport'
    scene = bpy.context.scene
    studio(scene, (3.6, -5.4, 3.0), (0.2, 0, 1.0), 4.6, (1200, 820))
    scene.render.filepath = str(OUT / 'Itaca_Interior_Preview.png')
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'Itaca_Interior_Preview.blend'))
    bpy.ops.render.render(write_still=True)


def main():
    config = json.loads(CONFIG_PATH.read_text(encoding='utf-8'))
    OUT.mkdir(parents=True, exist_ok=True)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    report = {'schema_version': 1, 'passed': False, 'stage': 'started',
              'blender_version': bpy.app.version_string,
              'generator_version': config['generator_version'], 'seed': config['seed'],
              'unreal_integration': 'not_checked_here',
              'assembly': config['assembly'],
              'config_sha256': hashlib.sha256(CONFIG_PATH.read_bytes()).hexdigest(),
              'generator_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
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
            result['fbx'] = export_asset(obj, OUT).relative_to(ROOT).as_posix()
            result['fbx_sha256'] = hashlib.sha256((ROOT / result['fbx']).read_bytes()).hexdigest()
        bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'Itaca_Ship_Source.blend'))
        # Validate the transport as well as the source; one mesh per FBX, correct metric bounds.
        for spec, result in zip(config['assets'], results):
            reset_scene()
            bpy.ops.import_scene.fbx(filepath=str(ROOT / result['fbx']))
            meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
            bpy.context.view_layer.update()
            ok = len(meshes) == 1 and all(abs(meshes[0].dimensions[i] - spec['dimensions_m'][i]) < 1e-4
                                          for i in range(3))
            result['fbx_roundtrip_passed'] = ok
            if not ok:
                raise RuntimeError('FBX object count/scale roundtrip failed: ' + spec['id'])

        # El volumen de estudio del plan maestro es 14 x 10 x 6 m: comprobarlo sobre el
        # montaje real, no sobre el casco suelto, o el contrato no dice nada.
        hull = spec_by_id(config, 'SM_Itaca_Hull_Blockout')['dimensions_m']
        engine = spec_by_id(config, 'SM_Itaca_Engine_Blockout')['dimensions_m']
        gear = spec_by_id(config, 'SM_Itaca_LandingGear_Blockout')['dimensions_m']
        antenna = spec_by_id(config, 'SM_Itaca_Antenna_Blockout')['dimensions_m']
        mount = config['assembly']
        base = mount['hull_mount_z_m']
        assembled = {
            'length_m': max(hull[0], 2 * abs(mount['engine_mount_x_m']) + engine[0]),
            'width_m': max(hull[1], 2 * (mount['engine_mount_y_m'] + engine[1] / 2),
                           2 * (mount['gear_mount_y_m'] + gear[1] / 2)),
            'height_m': max(base + hull[2], base + mount['antenna_mount_z_m'] + antenna[2])}
        for key, limit in [('length_m', 14.0), ('width_m', 10.0), ('height_m', 6.0)]:
            if assembled[key] > limit + 1e-6:
                raise RuntimeError('Assembly exceeds the study envelope: ' + key)
        # Los patines tienen que sostener el casco, no quedar enterrados bajo el vientre.
        if not (base < gear[2] <= base + 0.4):
            raise RuntimeError('Landing gear does not carry the hull clear of the ground')
        # La estancia interior mide 8 x 6 x 2,8 m: si el casco no la contiene, el exterior
        # y el interior dejan de ser el mismo objeto de ficción.
        if hull[0] < 8.0 or hull[1] < 6.0 or hull[2] < 2.8:
            raise RuntimeError('Hull cannot contain the 8 x 6 x 2.8 m room')
        report['assembled_envelope_m'] = assembled

        preview_exterior(config)
        preview_interior(config)
        report.update(passed=True, stage='source_export_roundtrip_preview_complete')
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        REPORT.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('ASTRAEON_SHIP: PASS ' + str(REPORT))


if __name__ == '__main__':
    main()
