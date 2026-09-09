"""Save the validated Itaca exterior and interior stations; fail closed on stale evidence.

Lands in the existing Itaca kit folder and reuses that kit's flat materials instead of
creating a parallel palette: these modules are the same family as the room they belong to.
"""
import hashlib
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/Astraeon/Art/Blockouts/Itaca'
CONFIG_PATH = 'Tools/Blender/configs/ship_blockout.json'


def digest(path):
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def import_mesh(fbx, name):
    options = unreal.FbxImportUI()
    for key, value in {'import_mesh': True, 'import_as_skeletal': False,
                       'import_materials': False, 'import_textures': False,
                       'automated_import_should_detect_type': False,
                       'mesh_type_to_import': unreal.FBXImportType.FBXIT_STATIC_MESH}.items():
        options.set_editor_property(key, value)
    data = options.get_editor_property('static_mesh_import_data')
    for key, value in {'import_uniform_scale': 1.0, 'convert_scene': True,
                       'convert_scene_unit': True, 'combine_meshes': False,
                       'auto_generate_collision': False}.items():
        data.set_editor_property(key, value)
    task = unreal.AssetImportTask()
    for key, value in {'filename': str(ROOT / fbx), 'destination_path': DEST,
                       'destination_name': name, 'automated': True, 'replace_existing': False,
                       'save': False, 'factory': unreal.FbxFactory(), 'options': options}.items():
        task.set_editor_property(key, value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    meshes = [a for a in task.get_objects() if isinstance(a, unreal.StaticMesh)]
    if len(meshes) != 1:
        raise RuntimeError('Expected exactly one mesh: ' + name)
    return meshes[0]


def main():
    report_path = ROOT / 'ContentPipeline/reports/ship_content_packages.json'
    report = {'passed': False, 'engine_version': unreal.SystemLibrary.get_engine_version(),
              'destination': DEST, 'saved_packages': True, 'gameplay_integrated': False,
              'meshes': [], 'materials_reused': []}
    try:
        config = json.loads((ROOT / CONFIG_PATH).read_text(encoding='utf-8'))
        validation = json.loads((ROOT / 'ContentPipeline/reports/ship_blockout_validation.json')
                                .read_text(encoding='utf-8'))
        if not validation['passed']:
            raise RuntimeError('Blender validation did not pass')
        if validation['config_sha256'] != digest(CONFIG_PATH):
            raise RuntimeError('Config changed since validation')
        if validation['generator_sha256'] != digest('Tools/Blender/generators/ship_blockout.py'):
            raise RuntimeError('Generator changed since validation')
        evidence = {item['id']: item for item in validation['assets']}

        # Preflight every input and destination before importing or saving anything.
        for spec in config['assets']:
            item = evidence[spec['id']]
            if not item['passed'] or not item['fbx_roundtrip_passed'] or digest(item['fbx']) != item['fbx_sha256']:
                raise RuntimeError('Invalid FBX evidence: ' + spec['id'])
            path = DEST + '/' + spec['id']
            if unreal.EditorAssetLibrary.does_asset_exist(path):
                existing = unreal.EditorAssetLibrary.load_asset(path)
                if unreal.EditorAssetLibrary.get_metadata_tag(existing, 'AstraeonSourceHash') != item['fbx_sha256']:
                    raise RuntimeError('Existing content differs; inspect before reimport: ' + path)
        materials = {}
        for spec in config['assets']:
            path = DEST + '/' + spec['material']
            if not unreal.EditorAssetLibrary.does_asset_exist(path):
                raise RuntimeError('The Itaca kit material is missing; run the kit import first: ' + path)
            materials[spec['material']] = unreal.EditorAssetLibrary.load_asset(path)

        unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
        pending = []
        for spec in config['assets']:
            item = evidence[spec['id']]
            path = DEST + '/' + spec['id']
            mesh = (unreal.EditorAssetLibrary.load_asset(path)
                    if unreal.EditorAssetLibrary.does_asset_exist(path)
                    else import_mesh(item['fbx'], spec['id']))
            bounds = mesh.get_bounds()
            size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
            expected = [d * 100 for d in spec['dimensions_m']]
            if not all(abs(a - b) < 0.1 for a, b in zip(size, expected)):
                raise RuntimeError('Incorrect centimetre dimensions: ' + path)
            if abs(bounds.origin.z - bounds.box_extent.z) > 0.1:
                raise RuntimeError('Incorrect ground pivot: ' + path)
            mesh.set_material(0, materials[spec['material']])
            unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonSourceHash', item['fbx_sha256'])
            unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonQuality', 'Q1_Blockout')
            pending.append((path, spec, size))
            report['meshes'].append({'id': spec['id'], 'asset': path, 'dimensions_cm': size,
                                     'material': spec['material'], 'fbx_sha256': item['fbx_sha256']})

        for path, spec, size in pending:
            if not unreal.EditorAssetLibrary.save_asset(path):
                raise RuntimeError('Could not save: ' + path)
        report['materials_reused'] = sorted(DEST + '/' + name for name in materials)
        report['assembly'] = validation['assembly']
        report['assembled_envelope_m'] = validation['assembled_envelope_m']
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    unreal.log('ASTRAEON_SHIP_PACKAGES: PASS')


if __name__ == '__main__':
    main()
