"""Save the validated human blockout as real Content packages; fail closed on stale evidence."""
import hashlib
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/Astraeon/Art/Blockouts/Human'
SKELETON_PATH = DEST + '/SKEL_Humanoid_A'
CONFIG_PATH = 'Tools/Blender/configs/humanoid_blockout.json'
MESH_IDS = ['SK_Human_Body_Blockout', 'SK_Human_HandsFP_Blockout']


def digest(path):
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def import_options(kind, skeleton=None):
    options = unreal.FbxImportUI()
    types = {'skeletal': unreal.FBXImportType.FBXIT_SKELETAL_MESH,
             'animation': unreal.FBXImportType.FBXIT_ANIMATION}
    for key, value in {'import_mesh': kind != 'animation', 'import_as_skeletal': True,
                       'import_animations': kind == 'animation', 'import_materials': False,
                       'import_textures': False, 'create_physics_asset': False,
                       'automated_import_should_detect_type': False,
                       'mesh_type_to_import': types[kind]}.items():
        options.set_editor_property(key, value)
    if skeleton:
        options.set_editor_property('skeleton', skeleton)
    data = options.get_editor_property('skeletal_mesh_import_data' if kind == 'skeletal'
                                       else 'anim_sequence_import_data')
    for key, value in {'import_uniform_scale': 1.0, 'convert_scene': True,
                       'convert_scene_unit': True}.items():
        data.set_editor_property(key, value)
    if kind == 'animation':
        data.set_editor_property('animation_length', unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        data.set_editor_property('use_default_sample_rate', True)
    return options


def import_asset(fbx, name, kind, skeleton=None):
    task = unreal.AssetImportTask()
    for key, value in {'filename': str(ROOT / fbx), 'destination_path': DEST,
                       'destination_name': name, 'automated': True, 'replace_existing': False,
                       'save': False, 'factory': unreal.FbxFactory(),
                       'options': import_options(kind, skeleton)}.items():
        task.set_editor_property(key, value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    expected = unreal.SkeletalMesh if kind == 'skeletal' else unreal.AnimSequence
    found = [a for a in task.get_objects() if isinstance(a, expected)]
    if len(found) != 1:
        raise RuntimeError('Expected exactly one ' + kind + ': ' + name)
    return found[0]


def flat_material(name, color, cache):
    """One unlit-looking flat master per palette slot; blockout presentation, not production."""
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
    roughness.set_editor_property('r', 0.65)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Could not save material: ' + path)
    cache[name] = material
    return material


def assign_materials(mesh, palette, cache):
    """Slot names come from the Blender material names carried by the FBX."""
    slots = mesh.get_editor_property('materials')
    names = []
    for slot in slots:
        name = str(slot.get_editor_property('material_slot_name'))
        key = name.replace('M_Human_', '')
        if key not in palette:
            raise RuntimeError('Unmapped material slot: ' + name)
        slot.set_editor_property('material_interface', flat_material(name, palette[key], cache))
        names.append(name)
    mesh.set_editor_property('materials', slots)
    return names


def main():
    report_path = ROOT / 'ContentPipeline/reports/humanoid_content_packages.json'
    report = {'passed': False, 'engine_version': unreal.SystemLibrary.get_engine_version(),
              'destination': DEST, 'saved_packages': True, 'gameplay_integrated': False,
              'meshes': [], 'animations': [], 'materials': []}
    try:
        config = json.loads((ROOT / CONFIG_PATH).read_text(encoding='utf-8'))
        validation = json.loads((ROOT / 'ContentPipeline/reports/humanoid_blockout_validation.json')
                                .read_text(encoding='utf-8'))
        if not validation['passed']:
            raise RuntimeError('Blender validation did not pass')
        if validation['config_sha256'] != digest(CONFIG_PATH):
            raise RuntimeError('Config changed since validation')
        if validation['generator_sha256'] != digest('Tools/Blender/generators/humanoid_blockout.py'):
            raise RuntimeError('Generator changed since validation')
        if validation['validator_sha256'] != digest('Tools/Blender/validators/humanoid.py'):
            raise RuntimeError('Validator changed since validation')
        files = {item['id']: item for item in validation['files']}
        for item in files.values():
            if item['sha256'] != digest(item['path']):
                raise RuntimeError('Stale FBX: ' + item['id'])
        animation_specs = {entry['id']: entry for entry in validation['animations']}

        # Preflight every destination before importing or saving anything.
        for identifier, item in files.items():
            path = DEST + '/' + identifier
            if unreal.EditorAssetLibrary.does_asset_exist(path):
                existing = unreal.EditorAssetLibrary.load_asset(path)
                if unreal.EditorAssetLibrary.get_metadata_tag(existing, 'AstraeonSourceHash') != item['sha256']:
                    raise RuntimeError('Existing content differs; inspect before reimport: ' + path)
        if unreal.EditorAssetLibrary.does_asset_exist(SKELETON_PATH):
            raise RuntimeError('Skeleton already exists; delete or reuse deliberately: ' + SKELETON_PATH)

        unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
        cache = {}
        palette = config['palette']

        body = import_asset(files[MESH_IDS[0]]['path'], MESH_IDS[0], 'skeletal')
        skeleton = body.get_editor_property('skeleton')
        if not unreal.EditorAssetLibrary.rename_loaded_asset(skeleton, SKELETON_PATH):
            raise RuntimeError('Could not rename skeleton to ' + SKELETON_PATH)
        bones = skeleton.get_editor_property('bone_tree')
        hands = import_asset(files[MESH_IDS[1]]['path'], MESH_IDS[1], 'skeletal', skeleton)

        for mesh, identifier in ((body, MESH_IDS[0]), (hands, MESH_IDS[1])):
            if mesh.get_editor_property('skeleton') != skeleton:
                raise RuntimeError('Mesh bound to a different skeleton: ' + identifier)
            bounds = mesh.get_bounds()
            size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
            names = assign_materials(mesh, palette, cache)
            unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonSourceHash', files[identifier]['sha256'])
            unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonQuality', 'Q1_Blockout')
            report['meshes'].append({'id': identifier, 'asset': DEST + '/' + identifier,
                                     'bounds_cm': size, 'material_slots': names,
                                     'sha256': files[identifier]['sha256']})
        if abs(report['meshes'][0]['bounds_cm'][2] - config['height_m'] * 100) > 1.0:
            raise RuntimeError('Body height is not the canonical 180 cm')

        for identifier, spec in animation_specs.items():
            sequence = import_asset(files[identifier]['path'], identifier, 'animation', skeleton)
            duration = unreal.AnimationLibrary.get_sequence_length(sequence)
            expected = (spec['frames'] - 1) / config['fps']
            if abs(duration - expected) > 0.002:
                raise RuntimeError('Duration mismatch: ' + identifier)
            unreal.EditorAssetLibrary.set_metadata_tag(sequence, 'AstraeonSourceHash', files[identifier]['sha256'])
            report['animations'].append({'id': identifier, 'asset': DEST + '/' + identifier,
                                         'duration_seconds': duration, 'loop': spec['loop'],
                                         'sha256': files[identifier]['sha256']})

        report['materials'] = sorted(DEST + '/' + name for name in cache)
        report['skeleton'] = {'asset': SKELETON_PATH, 'bones': len(bones)}
        if len(bones) != validation['source']['bones']:
            raise RuntimeError('Bone count changed on import')

        for path in [SKELETON_PATH] + [entry['asset'] for entry in report['meshes'] + report['animations']]:
            if not unreal.EditorAssetLibrary.save_asset(path):
                raise RuntimeError('Could not save: ' + path)
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    unreal.log('ASTRAEON_HUMANOID_PACKAGES: PASS')


if __name__ == '__main__':
    main()
