"""Transient FBX import smoke. Saves a JSON report, never a map or imported package."""
import hashlib
import json
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
DESTINATION = '/Game/Astraeon/ArtValidation/ItacaBlockout'


def main():
    report_path = ROOT / 'ContentPipeline/reports/itaca_unreal_import.json'
    report = {'schema_version': 1, 'passed': False, 'saved_packages': False,
              'engine_version': unreal.SystemLibrary.get_engine_version(), 'assets': [],
              'scope': 'transient_import_dimensions_only_not_collision_or_gameplay'}
    try:
        config = json.loads((ROOT / 'Tools/Blender/configs/itaca_blockout.json').read_text(encoding='utf-8'))
        validation = json.loads((ROOT / 'ContentPipeline/reports/itaca_blockout_validation.json').read_text(encoding='utf-8'))
        if not validation['passed']:
            raise RuntimeError('Blender source validation must pass first')
        source_evidence = {item['id']: item for item in validation['assets']}
        if unreal.EditorAssetLibrary.does_directory_exist(DESTINATION):
            raise RuntimeError('Refusing to replace existing content in validation destination')
        # Use the installed legacy FBX importer explicitly for reproducible options.
        unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
        for spec in config['assets']:
            path = ROOT / 'ContentPipeline/Generated/ItacaBlockout' / (spec['id'] + '.fbx')
            if hashlib.sha256(path.read_bytes()).hexdigest() != source_evidence[spec['id']]['fbx_sha256']:
                raise RuntimeError('FBX changed after Blender validation: ' + spec['id'])
            options = unreal.FbxImportUI()
            options.set_editor_property('import_mesh', True)
            options.set_editor_property('import_as_skeletal', False)
            options.set_editor_property('import_materials', False)
            options.set_editor_property('import_textures', False)
            options.set_editor_property('automated_import_should_detect_type', False)
            options.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)
            data = options.get_editor_property('static_mesh_import_data')
            data.set_editor_property('import_uniform_scale', 1.0)
            data.set_editor_property('convert_scene', True)
            data.set_editor_property('convert_scene_unit', True)
            data.set_editor_property('combine_meshes', False)
            data.set_editor_property('auto_generate_collision', False)
            task = unreal.AssetImportTask()
            task.set_editor_property('filename', str(path))
            task.set_editor_property('destination_path', DESTINATION)
            task.set_editor_property('destination_name', spec['id'])
            task.set_editor_property('automated', True)
            task.set_editor_property('replace_existing', False)
            task.set_editor_property('save', False)
            task.set_editor_property('factory', unreal.FbxFactory())
            task.set_editor_property('options', options)
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            assets = task.get_objects()
            meshes = [obj for obj in assets if isinstance(obj, unreal.StaticMesh)]
            if len(meshes) != 1:
                raise RuntimeError('Expected one StaticMesh: ' + spec['id'])
            bounds = meshes[0].get_bounds()
            extent = bounds.box_extent
            actual = [extent.x * 2, extent.y * 2, extent.z * 2]
            expected = [d * 100 for d in spec['dimensions_m']]
            passed = all(abs(a - b) < 0.1 for a, b in zip(actual, expected))
            report['assets'].append({'id': spec['id'], 'passed': passed,
                'dimensions_cm': actual, 'expected_dimensions_cm': expected,
                'fbx_sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
            if not passed:
                raise RuntimeError('Unreal centimetre bounds mismatch: ' + spec['id'])
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    unreal.log('ASTRAEON_ART_IMPORT: PASS')


if __name__ == '__main__':
    main()
