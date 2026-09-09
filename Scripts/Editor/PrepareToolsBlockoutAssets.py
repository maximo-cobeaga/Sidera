"""Save the validated handheld art as Content packages, bound to the saved SKEL_Humanoid_A."""
import hashlib
import json
from pathlib import Path
import unreal
import sys
sys.path.insert(0, str(Path(__file__).parent))
from CharacterAnimationScale import normalize_root_scale, validate_pose_scale

ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/Astraeon/Art/Blockouts/Tools'
SKELETON_PATH = '/Game/Astraeon/Art/Blockouts/Human/SKEL_Humanoid_A'
CONFIG_PATH = 'Tools/Blender/configs/tools_blockout.json'


def digest(path):
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def import_asset(fbx, name, kind, skeleton=None):
    options = unreal.FbxImportUI()
    types = {'static': unreal.FBXImportType.FBXIT_STATIC_MESH,
             'animation': unreal.FBXImportType.FBXIT_ANIMATION}
    for key, value in {'import_mesh': kind != 'animation', 'import_as_skeletal': kind != 'static',
                       'import_animations': kind == 'animation', 'import_materials': False,
                       'import_textures': False, 'create_physics_asset': False,
                       'automated_import_should_detect_type': False,
                       'mesh_type_to_import': types[kind]}.items():
        options.set_editor_property(key, value)
    if skeleton:
        options.set_editor_property('skeleton', skeleton)
    data = options.get_editor_property('static_mesh_import_data' if kind == 'static'
                                       else 'anim_sequence_import_data')
    for key, value in {'import_uniform_scale': 1.0, 'convert_scene': True,
                       'convert_scene_unit': True}.items():
        data.set_editor_property(key, value)
    if kind == 'static':
        data.set_editor_property('combine_meshes', False)
        data.set_editor_property('auto_generate_collision', False)
    else:
        data.set_editor_property('animation_length', unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        data.set_editor_property('use_default_sample_rate', True)
    task = unreal.AssetImportTask()
    for key, value in {'filename': str(ROOT / fbx), 'destination_path': DEST,
                       'destination_name': name, 'automated': True, 'replace_existing': False,
                       'save': False, 'factory': unreal.FbxFactory(), 'options': options}.items():
        task.set_editor_property(key, value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    expected = unreal.StaticMesh if kind == 'static' else unreal.AnimSequence
    found = [a for a in task.get_objects() if isinstance(a, expected)]
    if len(found) != 1:
        raise RuntimeError('Expected exactly one ' + kind + ': ' + name)
    if kind == 'animation':
        normalize_root_scale(found[0])
        validate_pose_scale(found[0])
    return found[0]


def flat_material(name, color, cache):
    if name in cache:
        return cache[name]
    path = DEST + '/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        cache[name] = unreal.EditorAssetLibrary.load_asset(path)
        return cache[name]
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    base = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector)
    base.set_editor_property('constant', unreal.LinearColor(color[0], color[1], color[2], 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant)
    roughness.set_editor_property('r', 0.55)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Could not save material: ' + path)
    cache[name] = material
    return material


def assign_materials(mesh, palette, cache):
    slots = mesh.get_editor_property('static_materials')
    names = []
    for index, slot in enumerate(slots):
        name = str(slot.get_editor_property('material_slot_name'))
        key = name.replace('M_Tool_', '')
        if key not in palette:
            raise RuntimeError('Unmapped material slot: ' + name)
        mesh.set_material(index, flat_material(name, palette[key], cache))
        names.append(name)
    return names


def main():
    report_path = ROOT / 'ContentPipeline/reports/tools_content_packages.json'
    report = {'passed': False, 'engine_version': unreal.SystemLibrary.get_engine_version(),
              'destination': DEST, 'saved_packages': True, 'gameplay_integrated': False,
              'meshes': [], 'animations': [], 'materials': []}
    try:
        config = json.loads((ROOT / CONFIG_PATH).read_text(encoding='utf-8'))
        validation = json.loads((ROOT / 'ContentPipeline/reports/tools_blockout_validation.json')
                                .read_text(encoding='utf-8'))
        if not validation['passed']:
            raise RuntimeError('Blender validation did not pass')
        for key, path in [('config_sha256', CONFIG_PATH),
                          ('generator_sha256', 'Tools/Blender/generators/tools_blockout.py'),
                          ('validator_sha256', 'Tools/Blender/validators/tools.py')]:
            if validation[key] != digest(path):
                raise RuntimeError('Changed since validation: ' + path)
        for item in validation['assets'] + validation['animation_files']:
            if item['sha256'] != digest(item['path']):
                raise RuntimeError('Stale FBX: ' + item['id'])
        if not unreal.EditorAssetLibrary.does_asset_exist(SKELETON_PATH):
            raise RuntimeError('Run PrepareHumanoidBlockoutAssets.py first: ' + SKELETON_PATH)
        skeleton = unreal.EditorAssetLibrary.load_asset(SKELETON_PATH)

        # Preflight every destination before importing or saving anything.
        for item in validation['assets'] + validation['animation_files']:
            path = DEST + '/' + item['id']
            if unreal.EditorAssetLibrary.does_asset_exist(path):
                existing = unreal.EditorAssetLibrary.load_asset(path)
                if unreal.EditorAssetLibrary.get_metadata_tag(existing, 'AstraeonSourceHash') != item['sha256']:
                    raise RuntimeError('Existing content differs; inspect before reimport: ' + path)

        unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
        cache = {}
        palette = config['palette']

        for item in validation['assets']:
            mesh = import_asset(item['path'], item['id'], 'static')
            bounds = mesh.get_bounds()
            size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
            expected = [d * 100 for d in item['dimensions_m']]
            if not all(abs(a - b) < 0.1 for a, b in zip(size, expected)):
                raise RuntimeError('Incorrect centimetre dimensions: ' + item['id'])
            # convert_scene mirrors Y going from Blender's right-handed frame to Unreal's.
            centre = [(a + b) * 50 for a, b in zip(item['bounds_min_m'], item['bounds_max_m'])]
            centre[1] = -centre[1]
            origin = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
            if not all(abs(a - b) < 0.1 for a, b in zip(origin, centre)):
                raise RuntimeError('Grip pivot moved on import: ' + item['id'])
            names = assign_materials(mesh, palette, cache)
            unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonSourceHash', item['sha256'])
            unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonQuality', 'Q1_Blockout')
            if item.get('item_id'):
                unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonItemId', item['item_id'])
            report['meshes'].append({'id': item['id'], 'asset': DEST + '/' + item['id'],
                                     'item_id': item.get('item_id'), 'dimensions_cm': size,
                                     'grip_origin_cm': origin, 'material_slots': names,
                                     'sha256': item['sha256']})

        expected_seconds = (config['animation_frames'] - 1) / config['fps']
        for item in validation['animation_files']:
            sequence = import_asset(item['path'], item['id'], 'animation', skeleton)
            if sequence.get_editor_property('skeleton') != skeleton:
                raise RuntimeError('Animation bound to a different skeleton: ' + item['id'])
            duration = unreal.AnimationLibrary.get_sequence_length(sequence)
            if abs(duration - expected_seconds) > 0.002:
                raise RuntimeError('Duration mismatch: ' + item['id'])
            unreal.EditorAssetLibrary.set_metadata_tag(sequence, 'AstraeonSourceHash', item['sha256'])
            report['animations'].append({'id': item['id'], 'asset': DEST + '/' + item['id'],
                                         'duration_seconds': duration, 'sha256': item['sha256']})

        report['materials'] = sorted(DEST + '/' + name for name in cache)
        report['attachment'] = validation['attachment']
        for path in [entry['asset'] for entry in report['meshes'] + report['animations']]:
            if not unreal.EditorAssetLibrary.save_asset(path):
                raise RuntimeError('Could not save: ' + path)
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    unreal.log('ASTRAEON_TOOLS_PACKAGES: PASS')


if __name__ == '__main__':
    main()
