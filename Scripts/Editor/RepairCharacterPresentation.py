"""Idempotent repair of the existing player animation and material packages."""
import json
import sys
from pathlib import Path
import unreal

sys.path.insert(0, str(Path(__file__).parent))
from CharacterAnimationScale import normalize_root_scale, validate_pose_scale

root = Path(unreal.Paths.project_dir())
report = dict(passed=False, animations=[], materials=[])
try:
    directories = ('/Game/Astraeon/Characters/Player/Optimized',
                   '/Game/Astraeon/Art/Blockouts/Human', '/Game/Astraeon/Art/Blockouts/Tools')
    assets = [unreal.load_asset(p) for d in directories for p in unreal.EditorAssetLibrary.list_assets(d)]
    animations = [a for a in assets if isinstance(a, unreal.AnimSequence) and not a.get_name().startswith('AN_HandsFP_')]
    assert len(animations) == 59, f'Expected 45 body + 14 hand clips, got {len(animations)}'
    for anim in animations:
        changed = normalize_root_scale(anim)
        heights = validate_pose_scale(anim)
        if changed:
            assert unreal.EditorAssetLibrary.save_loaded_asset(anim)
        report['animations'].append(dict(asset=anim.get_path_name(), repaired=changed, head_heights_cm=heights))
    # Ready arms come from the already-authored scanner grip. Keep locomotion in the
    # torso/legs, while shoulders stay behind the eye and hands remain in the viewport.
    scan = unreal.load_asset('/Game/Astraeon/Art/Blockouts/Tools/AN_Human_Scan_Tool_Blockout')
    ready = unreal.AnimPoseExtensions.get_anim_pose_at_time(scan, 0, unreal.AnimPoseEvaluationOptions())
    names = unreal.AnimPoseExtensions.get_bone_names(ready)
    arms = [n for n in names if str(n).startswith(('clavicle_', 'upperarm_', 'lowerarm_', 'hand_', 'thumb_', 'index_', 'middle_', 'ring_', 'pinky_'))]
    for kind in ('Idle', 'Walk', 'Run', 'Jump', 'Land'):
        dest = '/Game/Astraeon/Art/Blockouts/Human/AN_HandsFP_' + kind
        anim = unreal.load_asset(dest) if unreal.EditorAssetLibrary.does_asset_exist(dest) else unreal.EditorAssetLibrary.duplicate_asset(
            '/Game/Astraeon/Art/Blockouts/Human/AN_Human_' + kind + '_Blockout', dest)
        count = anim.get_editor_property('data_model_interface').get_number_of_keys()
        controller = anim.get_editor_property('controller')
        for name in arms:
            t = unreal.AnimPoseExtensions.get_bone_pose(ready, name, unreal.AnimPoseSpaces.LOCAL)
            assert controller.set_bone_track_keys(name, [t.translation]*count, [t.rotation]*count, [t.scale3d]*count, False)
        heights = validate_pose_scale(anim)
        assert unreal.EditorAssetLibrary.save_loaded_asset(anim)
        report['animations'].append(dict(asset=anim.get_path_name(), ready_pose=True, head_heights_cm=heights))
    for material in [a for a in assets if isinstance(a, unreal.Material)]:
        if material.get_name().startswith(('M_Player_', 'M_Human_')):
            unreal.MaterialEditingLibrary.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
            if material.get_name().startswith('M_Player_'):
                unreal.MaterialEditingLibrary.set_material_usage(material, unreal.MaterialUsage.MATUSAGE_MORPH_TARGETS)
            unreal.MaterialEditingLibrary.recompile_material(material)
            assert unreal.EditorAssetLibrary.save_loaded_asset(material)
            report['materials'].append(material.get_path_name())
    report['passed'] = True
    unreal.log('ASTRAEON_CHARACTER_REPAIR: PASS')
finally:
    (root / 'ContentPipeline/reports/character_presentation_repair.json').write_text(json.dumps(report, indent=2) + '\n')
