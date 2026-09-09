"""Transient skeletal FBX/animation test. Never changes maps or gameplay packages."""
import hashlib
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/Astraeon/ArtValidation/HumanoidBlockout'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    report_path = ROOT / 'ContentPipeline/reports/humanoid_unreal_import.json'
    report = {'passed': False, 'saved_packages': False, 'gameplay_integration': False,
              'engine_version': unreal.SystemLibrary.get_engine_version(), 'assets': []}
    try:
        source = json.loads((ROOT / 'ContentPipeline/reports/humanoid_blockout_validation.json').read_text(encoding='utf-8'))
        config_path = ROOT / 'Tools/Blender/configs/humanoid_blockout.json'
        config = json.loads(config_path.read_text(encoding='utf-8'))
        assert source['passed'] and source['config_sha256'] == digest(config_path), 'Stale source validation'
        assert source['generator_sha256'] == digest(ROOT / 'Tools/Blender/generators/humanoid_blockout.py')
        assert source['validator_sha256'] == digest(ROOT / 'Tools/Blender/validators/humanoid.py')
        for item in source['files']:
            assert digest(ROOT / item['path']) == item['sha256'], 'Stale FBX: ' + item['id']
        assert not unreal.EditorAssetLibrary.does_directory_exist(DEST), 'Refusing to overwrite existing content'
        unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
        skeleton = None
        for item in source['files']:
            is_mesh = item['id'].startswith('SK_')
            options = unreal.FbxImportUI()
            for key, value in {'import_mesh': is_mesh, 'import_as_skeletal': True,
                               'import_animations': not is_mesh, 'import_materials': False,
                               'import_textures': False, 'create_physics_asset': False,
                               'automated_import_should_detect_type': False,
                               'mesh_type_to_import': unreal.FBXImportType.FBXIT_SKELETAL_MESH if is_mesh else unreal.FBXImportType.FBXIT_ANIMATION}.items():
                options.set_editor_property(key, value)
            if skeleton:
                options.set_editor_property('skeleton', skeleton)
            data = options.get_editor_property('skeletal_mesh_import_data' if is_mesh else 'anim_sequence_import_data')
            for key, value in {'import_uniform_scale': 1.0, 'convert_scene': True, 'convert_scene_unit': True}.items():
                data.set_editor_property(key, value)
            if is_mesh:
                data.set_editor_property('use_t0_as_ref_pose', False)
                data.set_editor_property('update_skeleton_reference_pose', False)
            else:
                data.set_editor_property('animation_length', unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
                data.set_editor_property('use_default_sample_rate', True)
            task = unreal.AssetImportTask()
            for key, value in {'filename': str(ROOT / item['path']), 'destination_path': DEST,
                               'destination_name': item['id'], 'automated': True,
                               'replace_existing': False, 'save': False,
                               'factory': unreal.FbxFactory(), 'options': options}.items():
                task.set_editor_property(key, value)
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            expected_type = unreal.SkeletalMesh if is_mesh else unreal.AnimSequence
            assets = [a for a in task.get_objects() if isinstance(a, expected_type)]
            assert len(assets) == 1, 'Expected one imported asset: ' + item['id']
            asset = assets[0]
            row = {'id': item['id'], 'sha256': item['sha256'], 'passed': True}
            if is_mesh:
                actual_skeleton = asset.get_editor_property('skeleton')
                if skeleton is None:
                    skeleton = actual_skeleton
                assert actual_skeleton == skeleton, 'FP/body skeleton mismatch'
                bounds = asset.get_bounds()
                row['dimensions_cm'] = [2 * bounds.box_extent.x, 2 * bounds.box_extent.y, 2 * bounds.box_extent.z]
                if 'Body' in item['id']:
                    assert abs(row['dimensions_cm'][2] - 180) < .1, 'Centimetre scale mismatch'
            else:
                assert asset.get_editor_property('skeleton') == skeleton
                kind = item['id'].removeprefix('AN_Human_').removesuffix('_Blockout')
                duration = unreal.AnimationLibrary.get_sequence_length(asset)
                expected_duration = (config['animations'][kind]['frames'] - 1) / config['fps']
                assert abs(duration - expected_duration) < .002, 'Animation duration mismatch'
                row['duration_seconds'] = duration
                poses = [unreal.AnimPoseExtensions.get_anim_pose_at_time(asset, t, unreal.AnimPoseEvaluationOptions())
                         for t in [0, duration * .25, duration]]
                names = unreal.AnimPoseExtensions.get_bone_names(poses[0])
                assert len(names) == source['source']['bones'], 'Extra or missing imported bones'
                assert str(names[0]) == 'root', 'Imported root must be the authored ground root'
                assert all(name in [str(n) for n in names] for name in ['root', 'hand_l', 'hand_r', 'socket_tool_r'])
                def transform_values(p, name):
                    tr = unreal.AnimPoseExtensions.get_bone_pose(p, name, unreal.AnimPoseSpaces.LOCAL)
                    return [tr.translation.x, tr.translation.y, tr.translation.z,
                            tr.rotation.x, tr.rotation.y, tr.rotation.z, tr.rotation.w]
                differences = []
                for name in names:
                    start = transform_values(poses[0], name)
                    middle = transform_values(poses[1], name)
                    differences.append(max(abs(a-b) for a,b in zip(start, middle)))
                    if config['animations'][kind]['loop']:
                        end = transform_values(poses[2], name)
                        assert max(abs(a-b) for a,b in zip(start, end)) < .002, 'Imported loop discontinuity'
                assert max(differences) > .001, 'Imported animation is static'
                row['evaluated_bones'] = len(names)
                row['evaluated_motion'] = True
            report['assets'].append(row)
        report['passed'] = True
    except Exception as exc:
        report['error'] = str(exc)
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    unreal.log('ASTRAEON_HUMANOID_IMPORT: PASS')


if __name__ == '__main__':
    main()
