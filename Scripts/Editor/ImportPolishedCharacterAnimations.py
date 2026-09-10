"""Importa el lote de animaciones pulidas en una ruta Unreal aislada.

El lote anterior se conserva en ``/Optimized``. Este importador sólo crea la
variante ``/Optimized_Polished`` y falla si la carpeta ya existe, para evitar
que una repetición accidental sobrescriba una comparación anterior.
"""
import hashlib
import json
import sys
import traceback
from pathlib import Path

import unreal


SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))


ROOT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = ROOT / 'graphics/characters/main_player/exports/AN_Astraeon_Player_All_Polished_20260910.fbx'
DEST = '/Game/Astraeon/Characters/Player/Optimized_Polished'
SKELETON_PATH = '/Game/Astraeon/Characters/Player/Optimized/SK_Astraeon_Player_Skeleton'
REPORT = ROOT / 'ContentPipeline/reports/polished_character_animation_import.json'
ALLOW_REIMPORT = '-AstraeonAllowPolishedReimport' in unreal.SystemLibrary.get_command_line()
REQUIRED_BODY_CLIPS = {
    'Idle', 'Jump_Start', 'Jump_Loop', 'Jump_Land',
    'Walk_F', 'Walk_B', 'Walk_L', 'Walk_R',
    'Run_F', 'Run_B', 'Run_L', 'Run_R',
    'Scan', 'Interact', 'Pickup', 'Tool_Use', 'UseTool', 'Inspect',
}


def import_animations():
    options = unreal.FbxImportUI()
    properties = dict(
        import_mesh=False,
        import_as_skeletal=True,
        import_animations=True,
        import_materials=False,
        import_textures=False,
        create_physics_asset=False,
        automated_import_should_detect_type=False,
        mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION,
    )
    for key, value in properties.items():
        options.set_editor_property(key, value)
    skeleton = unreal.load_asset(SKELETON_PATH)
    if not skeleton:
        raise RuntimeError('No se encontró el esqueleto aprobado: ' + SKELETON_PATH)
    options.set_editor_property('skeleton', skeleton)
    data = options.get_editor_property('anim_sequence_import_data')
    for key, value in dict(
        import_uniform_scale=1.0,
        convert_scene=True,
        convert_scene_unit=True,
    ).items():
        data.set_editor_property(key, value)
    data.set_editor_property(
        'animation_length', unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    data.set_editor_property('use_default_sample_rate', True)

    task = unreal.AssetImportTask()
    for key, value in dict(
        filename=str(SOURCE),
        destination_path=DEST,
        destination_name='AN_Astraeon_Player_All',
        automated=True,
        replace_existing=ALLOW_REIMPORT,
        save=False,
        factory=unreal.FbxFactory(),
        options=options,
    ).items():
        task.set_editor_property(key, value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return skeleton


def main():
    report = dict(
        passed=False,
        saved_packages=False,
        engine_version=unreal.SystemLibrary.get_engine_version(),
        source=str(SOURCE),
        destination=DEST,
        required_body_clips=sorted(REQUIRED_BODY_CLIPS),
        sequences=[],
    )
    try:
        if not SOURCE.is_file():
            raise RuntimeError('No existe el FBX pulido: ' + str(SOURCE))
        if unreal.EditorAssetLibrary.does_directory_exist(DEST) and not ALLOW_REIMPORT:
            raise RuntimeError('La carpeta destino ya existe; se rechaza sobrescribir: ' + DEST)
        report['reimport'] = ALLOW_REIMPORT
        report['source_sha256'] = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
        unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
        skeleton = import_animations()
        assets = [unreal.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets(DEST)]
        animations = [a for a in assets if isinstance(a, unreal.AnimSequence)]
        names = {a.get_name() for a in animations}
        report['imported_count'] = len(animations)
        report['imported_names'] = sorted(names)
        from CharacterAnimationScale import normalize_root_scale, validate_pose_scale
        body = {}
        for clip_id in sorted(REQUIRED_BODY_CLIPS):
            suffix = 'AN_Player_' + clip_id
            matches = [a for a in animations if a.get_name().endswith(suffix)]
            if len(matches) != 1:
                raise RuntimeError('Clip requerido ausente o ambiguo: %s (%s)' % (clip_id, matches))
            anim = matches[0]
            normalize_root_scale(anim)
            validate_pose_scale(anim)
            body[clip_id] = anim.get_path_name()
        report['body_clips'] = body
        for anim in animations:
            if anim.get_editor_property('skeleton') != skeleton:
                raise RuntimeError('Esqueleto inesperado en ' + anim.get_path_name())
            if unreal.AnimationLibrary.get_sequence_length(anim) <= 0.0:
                raise RuntimeError('Animación sin duración: ' + anim.get_path_name())
            report['sequences'].append(dict(
                name=anim.get_name(),
                asset=anim.get_path_name(),
                duration_s=unreal.AnimationLibrary.get_sequence_length(anim),
            ))
            if not unreal.EditorAssetLibrary.save_loaded_asset(anim):
                raise RuntimeError('No se pudo guardar ' + anim.get_path_name())
        report['saved_packages'] = True
        report['passed'] = True
        unreal.log('ASTRAEON_POLISHED_CHARACTER_ANIMATIONS: PASS')
    except Exception:
        report['error'] = traceback.format_exc()
        unreal.log_error(report['error'])
        raise
    finally:
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
