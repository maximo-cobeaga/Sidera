"""Import validated original region art; matching provenance permits safe repeat runs."""
import hashlib
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/Astraeon/Art/Blockouts/Region'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    report_path = ROOT / 'ContentPipeline/reports/region_unreal_import.json'
    report = {'passed': False, 'engine_version': unreal.SystemLibrary.get_engine_version(), 'assets': []}
    try:
        config_path = ROOT / 'Tools/Blender/configs/region_blockout.json'
        config = json.loads(config_path.read_text(encoding='utf-8'))
        validation = json.loads((ROOT / 'ContentPipeline/reports/region_blockout_validation.json').read_text(encoding='utf-8'))
        if not validation['passed'] or validation['config_sha256'] != digest(config_path):
            raise RuntimeError('Missing or stale Blender validation')
        if validation['generator_sha256'] != digest(ROOT / 'Tools/Blender/generators/region_blockout.py'):
            raise RuntimeError('Generator changed since validation')
        evidence = {a['id']: a for a in validation['assets']}
        # Preflight all inputs before importing or saving anything.
        for spec in config['assets']:
            item = evidence[spec['id']]
            if not item['passed'] or not item['fbx_roundtrip_passed'] or digest(ROOT / item['fbx']) != item['fbx_sha256']:
                raise RuntimeError('Invalid FBX evidence: ' + spec['id'])
            path = DEST + '/' + spec['id']
            if unreal.EditorAssetLibrary.does_asset_exist(path):
                mesh = unreal.EditorAssetLibrary.load_asset(path)
                if unreal.EditorAssetLibrary.get_metadata_tag(mesh, 'AstraeonSourceHash') != item['fbx_sha256']:
                    raise RuntimeError('Existing content differs; inspect before reimport: ' + path)
        unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
        pending = []
        for spec in config['assets']:
            item = evidence[spec['id']]
            path = DEST + '/' + spec['id']
            if unreal.EditorAssetLibrary.does_asset_exist(path):
                mesh = unreal.EditorAssetLibrary.load_asset(path)
            else:
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
                for key, value in {'filename': str(ROOT / item['fbx']), 'destination_path': DEST,
                                   'destination_name': spec['id'], 'automated': True,
                                   'replace_existing': False, 'save': False,
                                   'factory': unreal.FbxFactory(), 'options': options}.items():
                    task.set_editor_property(key, value)
                unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
                meshes = [o for o in task.get_objects() if isinstance(o, unreal.StaticMesh)]
                if len(meshes) != 1:
                    raise RuntimeError('Expected one mesh: ' + spec['id'])
                mesh = meshes[0]
            bounds = mesh.get_bounds()
            size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
            expected = [d * 100 for d in spec['dimensions_m']]
            if not all(abs(a - b) < 0.1 for a, b in zip(size, expected)):
                raise RuntimeError('Incorrect centimetre dimensions: ' + path)
            if abs(bounds.origin.z - bounds.box_extent.z) > 0.1:
                raise RuntimeError('Incorrect ground pivot: ' + path)
            pending.append((spec, mesh, item))
            report['assets'].append({'id': spec['id'], 'asset': path, 'passed': True,
                                      'dimensions_cm': size, 'fbx_sha256': item['fbx_sha256']})
        for spec, mesh, item in pending:
            mat_path = DEST + '/' + spec['material']
            if unreal.EditorAssetLibrary.does_asset_exist(mat_path):
                mat = unreal.EditorAssetLibrary.load_asset(mat_path)
            else:
                mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                    spec['material'], DEST, unreal.Material, unreal.MaterialFactoryNew())
                color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
                color.set_editor_property('constant', unreal.LinearColor(*spec['color'], 1.0))
                unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
                roughness = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant)
                roughness.set_editor_property('r', 0.7)
                unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
                unreal.MaterialEditingLibrary.recompile_material(mat)
                if not unreal.EditorAssetLibrary.save_loaded_asset(mat):
                    raise RuntimeError('Could not save material: ' + mat_path)
            mesh.set_material(0, mat)
            unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonSourceHash', item['fbx_sha256'])
            if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
                raise RuntimeError('Could not save mesh: ' + spec['id'])
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    unreal.log('ASTRAEON_REGION_IMPORT: PASS')


if __name__ == '__main__':
    main()
