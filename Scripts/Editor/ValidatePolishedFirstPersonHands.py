"""Verifica que los clips FP pulidos tengan movimiento de brazos real."""
import json
import math
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/Astraeon/Art/Blockouts/Human/PolishedFP/'
REPORT = ROOT / 'ContentPipeline/reports/polished_first_person_hands_validation.json'


def distance(a, b):
    # q and -q representan la misma rotación; usar el ángulo evita falsos
    # positivos cuando Unreal cambia el signo al comprimir una pista.
    dot = abs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w)
    return 2.0 * math.acos(max(-1.0, min(1.0, dot)))


def main():
    report = dict(passed=False, clips=[])
    for kind in ('Idle', 'Walk', 'Run', 'Jump', 'Land'):
        path = DEST + 'AN_HandsFP_Polished_' + kind
        anim = unreal.load_asset(path)
        if not anim:
            raise RuntimeError('Falta ' + path)
        length = unreal.AnimationLibrary.get_sequence_length(anim)
        first = unreal.AnimPoseExtensions.get_anim_pose_at_time(
            anim, 0.0, unreal.AnimPoseEvaluationOptions())
        middle = unreal.AnimPoseExtensions.get_anim_pose_at_time(
            anim, length * 0.25, unreal.AnimPoseEvaluationOptions())
        names = {str(n) for n in unreal.AnimPoseExtensions.get_bone_names(first)}
        motion = 0.0
        samples = {}
        for bone in ('clavicle_l', 'clavicle_r', 'upperarm_l', 'upperarm_r'):
            if bone not in names:
                raise RuntimeError('Falta hueso ' + bone + ' en ' + path)
            a = unreal.AnimPoseExtensions.get_bone_pose(first, bone, unreal.AnimPoseSpaces.LOCAL)
            b = unreal.AnimPoseExtensions.get_bone_pose(middle, bone, unreal.AnimPoseSpaces.LOCAL)
            motion = max(motion, distance(a.rotation, b.rotation))
            if bone == 'upperarm_l':
                samples = {
                    'first': [a.rotation.x, a.rotation.y, a.rotation.z, a.rotation.w],
                    'middle': [b.rotation.x, b.rotation.y, b.rotation.z, b.rotation.w],
                }
        if kind in ('Walk', 'Run', 'Jump', 'Land') and motion <= 0.0001:
            raise RuntimeError('Clip FP estático: ' + kind)
        report['clips'].append(dict(kind=kind, asset=anim.get_path_name(), duration_s=length,
                                    sampled_rotation_delta=motion, sample=samples))
    report['passed'] = True
    unreal.log('ASTRAEON_POLISHED_FIRST_PERSON_HANDS_VALIDATION: PASS')
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
