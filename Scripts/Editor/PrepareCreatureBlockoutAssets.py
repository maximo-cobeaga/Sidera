"""Save the validated quadruped as Content packages; fail closed on stale evidence."""
import hashlib
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/Astraeon/Art/Blockouts/Creatures'
SKELETON_PATH = DEST + '/SKEL_Quadruped_A'
CONFIG_PATH = 'Tools/Blender/configs/creature_blockout.json'


def digest(path):
    return hashlib.sha256((ROOT / path).read_bytes()).hexdigest()


def import_asset(fbx, name, kind, skeleton=None):
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
    task = unreal.AssetImportTask()
    for key, value in {'filename': str(ROOT / fbx), 'destination_path': DEST,
                       'destination_name': name, 'automated': True, 'replace_existing': False,
                       'save': False, 'factory': unreal.FbxFactory(), 'options': options}.items():
        task.set_editor_property(key, value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    expected = unreal.SkeletalMesh if kind == 'skeletal' else unreal.AnimSequence
    found = [a for a in task.get_objects() if isinstance(a, expected)]
    if len(found) != 1:
        raise RuntimeError('Expected exactly one ' + kind + ': ' + name)
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
    roughness.set_editor_property('r', 0.78)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Could not save material: ' + path)
    cache[name] = material
    return material


def assign_materials(mesh, spec, cache):
    """Slot names travel in the FBX from the Blender material names."""
    slots = mesh.get_editor_property('materials')
    names = []
    rebuilt = []
    for slot in slots:
        name = str(slot.get_editor_property('material_slot_name'))
        key = name.replace('M_Creature_', '').replace('_Plated', '').lower()
        if key not in spec['palette']:
            raise RuntimeError('Unmapped material slot: ' + name)
        entry = unreal.SkeletalMaterial()
        entry.set_editor_property('material_interface', flat_material(name, spec['palette'][key], cache))
        entry.set_editor_property('material_slot_name', name)
        rebuilt.append(entry)
        names.append(name)
    # Se construye la lista entera: mutar los structs que devuelve get_editor_property
    # opera sobre copias, así que la asignación no llegaba al paquete y las mallas se
    # guardaban con los slots en nulo. Ver Docs/AUDITORIA_ARTE.md.
    mesh.modify()
    mesh.set_editor_property('materials', rebuilt)
    written = list(mesh.get_editor_property('materials'))
    for index, slot_name in enumerate(names):
        assert written[index].get_editor_property('material_interface') is not None, 'Slot sin material: ' + slot_name
    return names


def main():
    report_path = ROOT / 'ContentPipeline/reports/creature_content_packages.json'
    report = {'passed': False, 'engine_version': unreal.SystemLibrary.get_engine_version(),
              'destination': DEST, 'saved_packages': True, 'gameplay_integrated': False,
              'meshes': [], 'animations': [], 'materials': []}
    try:
        config = json.loads((ROOT / CONFIG_PATH).read_text(encoding='utf-8'))
        validation = json.loads((ROOT / 'ContentPipeline/reports/creature_blockout_validation.json')
                                .read_text(encoding='utf-8'))
        if not validation['passed']:
            raise RuntimeError('Blender validation did not pass')
        for key, path in [('config_sha256', CONFIG_PATH),
                          ('generator_sha256', 'Tools/Blender/generators/creature_blockout.py'),
                          ('validator_sha256', 'Tools/Blender/validators/creature.py')]:
            if validation[key] != digest(path):
                raise RuntimeError('Changed since validation: ' + path)
        files = {item['id']: item for item in validation['files']}
        for item in files.values():
            if item['sha256'] != digest(item['path']):
                raise RuntimeError('Stale FBX: ' + item['id'])
        animations = {entry['id']: entry for entry in validation['animations']}

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
        skeleton = None
        for index, spec in enumerate(config['meshes']):
            identifier = spec['id']
            mesh = import_asset(files[identifier]['path'], identifier, 'skeletal', skeleton)
            if index == 0:
                skeleton = mesh.get_editor_property('skeleton')
                if not unreal.EditorAssetLibrary.rename_loaded_asset(skeleton, SKELETON_PATH):
                    raise RuntimeError('Could not rename skeleton to ' + SKELETON_PATH)
            elif mesh.get_editor_property('skeleton') != skeleton:
                raise RuntimeError('Variant bound to a different skeleton: ' + identifier)
            bounds = mesh.get_bounds()
            size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
            if abs(size[1] - config['length_m'] * 100) > 5.0:
                raise RuntimeError('Body length is off the design silhouette: ' + identifier)
            names = assign_materials(mesh, spec, cache)
            unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonSourceHash', files[identifier]['sha256'])
            unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonQuality', 'Q1_Blockout')
            unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonSpeciesId', config['species_id'])
            report['meshes'].append({'id': identifier, 'asset': DEST + '/' + identifier,
                                     'variant': spec['variant'], 'bounds_cm': size,
                                     'material_slots': names, 'sha256': files[identifier]['sha256']})

        bones = skeleton.get_editor_property('bone_tree')
        if len(bones) != validation['source']['bones']:
            raise RuntimeError('Bone count changed on import')
        report['skeleton'] = {'asset': SKELETON_PATH, 'bones': len(bones)}

        for identifier, spec in animations.items():
            sequence = import_asset(files[identifier]['path'], identifier, 'animation', skeleton)
            if sequence.get_editor_property('skeleton') != skeleton:
                raise RuntimeError('Animation bound to a different skeleton: ' + identifier)
            duration = unreal.AnimationLibrary.get_sequence_length(sequence)
            expected = (spec['frames'] - 1) / config['fps']
            if abs(duration - expected) > 0.002:
                raise RuntimeError('Duration mismatch: ' + identifier)
            unreal.EditorAssetLibrary.set_metadata_tag(sequence, 'AstraeonSourceHash', files[identifier]['sha256'])
            unreal.EditorAssetLibrary.set_metadata_tag(sequence, 'AstraeonAwarenessState', spec['state'])
            report['animations'].append({'id': identifier, 'asset': DEST + '/' + identifier,
                                         'duration_seconds': duration, 'loop': spec['loop'],
                                         'state': spec['state'], 'sha256': files[identifier]['sha256']})

        report['materials'] = sorted(DEST + '/' + name for name in cache)
        for path in [SKELETON_PATH] + [entry['asset'] for entry in report['meshes'] + report['animations']]:
            if not unreal.EditorAssetLibrary.save_asset(path):
                raise RuntimeError('Could not save: ' + path)
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    unreal.log('ASTRAEON_CREATURE_PACKAGES: PASS')


if __name__ == '__main__':
    main()
