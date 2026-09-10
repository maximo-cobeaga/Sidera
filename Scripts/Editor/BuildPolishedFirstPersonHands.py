"""Crea una variante aislada de las manos FP con locomoción visible.

Los clips AN_HandsFP originales mantienen la pose de agarre constante para
encuadrar las manos. Este paso conserva esa pose como base y añade un balanceo
pequeño y simétrico a clavículas/upperarms, sin sobrescribir los assets viejos.
"""
import json
import math
import sys
import traceback
from pathlib import Path

import unreal


SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))


ROOT = Path(unreal.Paths.project_dir()).resolve()
SOURCE_DIR = '/Game/Astraeon/Art/Blockouts/Human/'
DEST_DIR = '/Game/Astraeon/Art/Blockouts/Human/PolishedFP'
REPORT = ROOT / 'ContentPipeline/reports/polished_first_person_hands.json'
KINDS = ('Idle', 'Walk', 'Run', 'Jump', 'Land')
ARM_BONES = ('clavicle_l', 'clavicle_r', 'upperarm_l', 'upperarm_r')
READY_ASSET = '/Game/Astraeon/Art/Blockouts/Tools/AN_Human_Scan_Tool_Blockout'


def sample(anim, seconds):
    return unreal.AnimPoseExtensions.get_anim_pose_at_time(
        anim, seconds, unreal.AnimPoseEvaluationOptions())


def compose(delta, base):
    # Quat multiplication is exposed by the UE Python wrapper in 5.7.
    return delta * base


def swing(kind, bone, index, count):
    if count <= 1:
        phase = 0.0
    else:
        phase = index / float(count - 1)
    side = 1.0 if bone.endswith('_l') else -1.0
    if kind in ('Walk', 'Run'):
        amplitude = 4.0 if kind == 'Walk' else 6.0
        value = math.sin(math.tau * phase) * amplitude * side
    elif kind == 'Jump':
        value = math.sin(math.pi * phase) * 5.0
    elif kind == 'Land':
        value = math.sin(math.pi * phase) * -3.0
    else:
        value = math.sin(math.tau * phase) * 0.8
    if bone.startswith('clavicle_'):
        value *= 0.45
    return value


def main():
    report = dict(passed=False, destination=DEST_DIR, kinds=[], source_assets=[])
    try:
        if (unreal.EditorAssetLibrary.does_directory_exist(DEST_DIR)
                and unreal.EditorAssetLibrary.list_assets(DEST_DIR)):
            raise RuntimeError('La carpeta destino ya contiene assets; se rechaza sobrescribir: ' + DEST_DIR)
        ready = unreal.load_asset(READY_ASSET)
        if not ready:
            raise RuntimeError('No se encontró la pose base del escáner: ' + READY_ASSET)
        ready_pose = sample(ready, 0.0)
        ready_names = {str(n) for n in unreal.AnimPoseExtensions.get_bone_names(ready_pose)}
        missing = [name for name in ARM_BONES if name not in ready_names]
        if missing:
            raise RuntimeError('Faltan huesos de brazo en la pose base: ' + str(missing))

        unreal.EditorAssetLibrary.make_directory(DEST_DIR)
        for kind in KINDS:
            source_path = SOURCE_DIR + 'AN_HandsFP_' + kind
            source = unreal.load_asset(source_path)
            if not source:
                raise RuntimeError('Falta el clip fuente: ' + source_path)
            dest_path = DEST_DIR + '/AN_HandsFP_Polished_' + kind
            anim = unreal.EditorAssetLibrary.duplicate_asset(source_path, dest_path)
            if not anim:
                raise RuntimeError('No se pudo duplicar ' + source_path)
            model = anim.get_editor_property('data_model_interface')
            count = model.get_number_of_keys()
            duration = unreal.AnimationLibrary.get_sequence_length(anim)
            controller = anim.get_editor_property('controller')
            for bone in ARM_BONES:
                base = unreal.AnimPoseExtensions.get_bone_pose(
                    ready_pose, bone, unreal.AnimPoseSpaces.LOCAL)
                rotations = []
                for index in range(count):
                    delta = unreal.Rotator(
                        pitch=swing(kind, bone, index, count), yaw=0.0, roll=0.0).quaternion()
                    rotations.append(compose(delta, base.rotation))
                assert controller.set_bone_track_keys(
                    bone,
                    [base.translation] * count,
                    rotations,
                    [base.scale3d] * count,
                    False)
            from CharacterAnimationScale import normalize_root_scale, validate_pose_scale
            normalize_root_scale(anim)
            validate_pose_scale(anim)
            if not unreal.EditorAssetLibrary.save_loaded_asset(anim):
                raise RuntimeError('No se pudo guardar ' + anim.get_path_name())
            report['source_assets'].append(source.get_path_name())
            report['kinds'].append(dict(kind=kind, asset=anim.get_path_name(), keys=count,
                                        duration_s=duration))
        report['passed'] = True
        unreal.log('ASTRAEON_POLISHED_FIRST_PERSON_HANDS: PASS')
    except Exception:
        report['error'] = traceback.format_exc()
        unreal.log_error(report['error'])
        raise
    finally:
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
