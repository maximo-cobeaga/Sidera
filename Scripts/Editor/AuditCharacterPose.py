"""Read actual reference and animated bone transforms; no package writes."""
import json
from pathlib import Path
import unreal

result = []
for path in ('/Game/Astraeon/Characters/Player/Optimized/AN_Astraeon_Player_All_Armature_AN_Player_Idle',
             '/Game/Astraeon/Art/Blockouts/Human/AN_Human_Idle_Blockout'):
    anim = unreal.load_asset(path)
    skeleton = anim.get_editor_property('skeleton')
    ref = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, 0, unreal.AnimPoseEvaluationOptions())
    row = dict(asset=path, bones=[])
    for bone in ('root', 'pelvis', 'spine_01', 'head', 'hand_r'):
        entry = dict(bone=bone)
        for name, p in (('ref', ref), ('anim', pose)):
            for space in (unreal.AnimPoseSpaces.LOCAL, unreal.AnimPoseSpaces.WORLD):
                t = unreal.AnimPoseExtensions.get_bone_pose(p, bone, space)
                entry[name + str(space)] = str(t)
        row['bones'].append(entry)
    result.append(row)
Path(unreal.Paths.project_dir(), 'Saved/CharacterPoseAudit.json').write_text(json.dumps(result, indent=2))
