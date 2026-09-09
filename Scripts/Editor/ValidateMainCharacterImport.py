"""Import and evaluate the canonical protagonist in an isolated Unreal destination.

Run with PythonScriptPlugin and EditorScriptingUtilities enabled. Existing Q1
content is never touched. A failed validation does not save imported packages.
"""
import hashlib
import json
from pathlib import Path
import sys
import traceback
import unreal

# `-ExecutePythonScript` no anade la carpeta del script a sys.path, asi que sin
# esto falla `import MainCharacterAppearance` despues de importar malla y clips.
if str(Path(__file__).parent) not in sys.path:
    sys.path.insert(0, str(Path(__file__).parent))

ROOT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = ROOT / 'graphics/characters/main_player'
OPTIMIZED = '-AstraeonPlayerOptimized' in unreal.SystemLibrary.get_command_line()
DEST = '/Game/Astraeon/Characters/Player' + ('/Optimized' if OPTIMIZED else '')
EXPECTED_BONES = 75 if OPTIMIZED else 71
REPORT = ROOT / ('ContentPipeline/reports/main_character_unreal_optimized.json' if OPTIMIZED
                 else 'ContentPipeline/reports/main_character_unreal_import.json')


def import_fbx(filename, name, animation=False, skeleton=None):
    options = unreal.FbxImportUI()
    properties = dict(import_mesh=not animation, import_as_skeletal=True,
                      import_animations=animation, import_materials=False,
                      import_textures=False, create_physics_asset=False,
                      automated_import_should_detect_type=False,
                      mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION if animation
                      else unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    for key, value in properties.items():
        options.set_editor_property(key, value)
    if skeleton:
        options.set_editor_property('skeleton', skeleton)
    data = options.get_editor_property('anim_sequence_import_data' if animation
                                       else 'skeletal_mesh_import_data')
    for key, value in dict(import_uniform_scale=1.0, convert_scene=True,
                           convert_scene_unit=False).items():
        data.set_editor_property(key, value)
    if animation:
        data.set_editor_property('animation_length', unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        data.set_editor_property('use_default_sample_rate', True)
    else:
        data.set_editor_property('use_t0_as_ref_pose', False)
        data.set_editor_property('update_skeleton_reference_pose', False)
        data.set_editor_property('import_morph_targets', True)
    task = unreal.AssetImportTask()
    for key, value in dict(filename=str(filename), destination_path=DEST,
                           destination_name=name, automated=True, replace_existing=False,
                           save=False, factory=unreal.FbxFactory(), options=options).items():
        task.set_editor_property(key, value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return list(task.get_objects())


def main():
    report = dict(passed=False, saved_packages=False, gameplay_integrated=False,
                  engine_version=unreal.SystemLibrary.get_engine_version(), destination=DEST,
                  clips=[], source_hashes={})
    try:
        assert not unreal.EditorAssetLibrary.does_directory_exist(DEST), 'Destination exists; refusing overwrite'
        unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
        contract = json.loads((SOURCE / 'animations/animation_contract.json').read_text(encoding='utf-8'))
        for f in (SOURCE / 'exports').glob('*.fbx'):
            report['source_hashes'][f.name] = hashlib.sha256(f.read_bytes()).hexdigest()
        objects = import_fbx(SOURCE / 'exports/SK_Astraeon_Player.fbx', 'SK_Astraeon_Player')
        meshes = [o for o in objects if isinstance(o, unreal.SkeletalMesh)]
        assert len(meshes) == 1, 'Missing skeletal mesh'
        mesh = meshes[0]
        skeleton = mesh.get_editor_property('skeleton')
        bounds = mesh.get_bounds()
        dims = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
        report['dimensions_cm'] = dims
        assert abs(dims[2] - 183) < .25, f'Incorrect height: {dims}'
        assert abs(max(dims[:2]) - 183.72) < .5, f'Not the authored T pose: {dims}'
        subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
        report['skeletal_editor_api'] = [n for n in dir(subsystem) if any(k in n for k in ('lod', 'bone', 'socket', 'vertex'))]
        for lod in (1, 2, 3):
            assert subsystem.import_lod(mesh, lod, str(SOURCE / f'exports/SK_Astraeon_Player_LOD{lod}.fbx')), f'LOD{lod} import failed'
        report['lod_count'] = subsystem.get_lod_count(mesh)
        assert report['lod_count'] == 4
        import_fbx(SOURCE / 'exports/AN_Astraeon_Player_All.fbx',
                   'AN_Astraeon_Player_All', True, skeleton)
        # Legacy FBX factory returns only the last take in get_objects(). The
        # asset registry contains every take registered during the import.
        animations = [unreal.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets(DEST)]
        animations = [a for a in animations if isinstance(a, unreal.AnimSequence)]
        report['imported_animation_names'] = [o.get_name() for o in animations]
        assert len(animations) == len(contract['clips']) == 45, f'Expected 45 clips, got {len(animations)}'
        for clip in contract['clips']:
            candidates = [a for a in animations if a.get_name().endswith(clip['name'])]
            assert len(candidates) == 1, 'Cannot identify clip ' + clip['name']
            anim = candidates[0]
            from CharacterAnimationScale import normalize_root_scale, validate_pose_scale
            normalize_root_scale(anim)
            validate_pose_scale(anim)
            length = unreal.AnimationLibrary.get_sequence_length(anim)
            expected = clip.get('export_duration_s', clip['duration_s'])
            assert abs(length - expected) < .035, f'Duration {clip["name"]}: {length} != {expected}'
            poses = [unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, t, unreal.AnimPoseEvaluationOptions())
                     for t in (0, length * .25, length * .5, length * .75, length)]
            names = unreal.AnimPoseExtensions.get_bone_names(poses[0])
            assert len(names) == EXPECTED_BONES and str(names[0]) == 'root', 'Skeleton hierarchy changed'
            assert {'socket_tool_r', 'socket_helmet', 'socket_backpack', 'lowerarm_l'} <= {str(n) for n in names}
            def values(pose, bone):
                t = unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.LOCAL)
                return [t.translation.x, t.translation.y, t.translation.z,
                        t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w]
            movement = max(max(abs(a-b) for a,b in zip(values(poses[0], n), values(p, n)))
                           for n in names for p in poses[1:])
            seam = max(max(abs(a-b) for a,b in zip(values(poses[0], n), values(poses[-1], n))) for n in names)
            if clip['frames'] > 1:
                assert movement > .0001, 'Static animation: ' + clip['name']
            if clip['loop']:
                assert seam < .02, 'Loop discontinuity: ' + clip['name']
            row = dict(name=clip['name'], asset=anim.get_path_name(), duration_s=length,
                       bones=len(names), evaluated_motion=movement, loop_seam=seam)
            report['clips'].append(row)
        report['bones'] = EXPECTED_BONES
        if OPTIMIZED:
            from MainCharacterAppearance import finish_appearance
            report['appearance'] = finish_appearance(mesh, skeleton, DEST, SOURCE)
        # Persist only after actual imported animation evaluation and LOD checks.
        for asset in objects + animations:
            assert unreal.EditorAssetLibrary.save_loaded_asset(asset), 'Save failed: ' + asset.get_path_name()
        assert unreal.EditorAssetLibrary.save_loaded_asset(skeleton)
        report['saved_packages'] = True
        report['passed'] = True
        unreal.log('ASTRAEON_MAIN_CHARACTER_IMPORT: PASS')
    except Exception:
        report['error'] = traceback.format_exc()
        unreal.log_error(report['error'])
        raise
    finally:
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
