"""Calibración Blender → Unreal de la Fase 0 (ADR 0004).

Qué responde: si un metro de Blender llega a Unreal como 100 cm, con los ejes en su sitio, para
las cuatro clases de asset que el proyecto usa —malla estática, malla esquelética, una Action y
un shape key como morph target—.

Por qué hace falta aunque el pipeline ya funcione. El proyecto ya pagó una vez este error: la
importación de animaciones sueltas perdía la escala 100 del Armature y el protagonista quedaba
invisible (`Docs/INVESTIGACION_PERSONAJE_INVISIBLE.md`). Una escena de calibración convierte ese
fallo en algo que se detecta en dos minutos y no en dos sesiones.

Genera objetos con medidas conocidas y redondas a propósito: un cubo de 1 m mide 100 cm en Unreal
o el pipeline está roto, sin margen de interpretación.

Uso:
    & "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" --background ^
        --python Tools\\Blender\\calibration_ue57.py

Salida: FBX en `ContentPipeline/Generated/Calibration/` y un informe JSON en
`Docs/evidencia/calibration_blender_<fecha>.json`. Devuelve código distinto de cero si falla.
"""
import json
import math
import platform
import sys
from datetime import datetime
from pathlib import Path

import bpy
from mathutils import Vector

TOOL_ROOT = Path(__file__).resolve().parent
ROOT = TOOL_ROOT.parents[1]
sys.path.insert(0, str(TOOL_ROOT))

from presets.UE57_AST_V1 import (  # noqa: E402
    PRESET_NAME, PRESET_VERSION, describe,
    static_mesh_kwargs, skeletal_mesh_kwargs, animation_kwargs,
)

OUT_DIR = ROOT / 'ContentPipeline' / 'Generated' / 'Calibration'
EVIDENCE_DIR = ROOT / 'Docs' / 'evidencia'

# Medidas de referencia. Redondas para que un error de escala salte a la vista.
CUBE_SIZE_M = 1.0
MANNEQUIN_HEIGHT_M = 1.83   # La altura medida del protagonista, no un número inventado.
AXIS_LENGTH_M = 0.5

failures = []


def check(condition, message):
    if not condition:
        failures.append(message)
    return condition


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    # Métrico con Unit Scale 1.0 es la mitad del contrato: con otra escala de unidad, el FBX
    # sale con un factor que Unreal aplica en silencio.
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1.0
    scene.render.fps = 30


def verify_preset_has_not_drifted():
    """El preset es fuente única sólo si nadie mantiene una copia paralela.

    `main_character_export.py` define su propio `FBX_COMMON`. Aquí se comprueba que sigue siendo
    exactamente el del preset. Si alguien cambia uno y no el otro, esto falla ahora y no cuando
    un personaje aparezca rotado en Unreal.
    """
    try:
        import main_character_export as mce
    except Exception as exc:  # el módulo toca datos del personaje al importarse en algunos casos
        return {'checked': False, 'reason': 'no se pudo importar main_character_export: %s' % exc}

    expected = {k: v for k, v in skeletal_mesh_kwargs().items() if k != 'bake_anim'}
    actual = dict(mce.FBX_COMMON)

    differences = {}
    for key in sorted(set(expected) | set(actual)):
        if expected.get(key) != actual.get(key):
            differences[key] = {'preset': expected.get(key), 'main_character_export': actual.get(key)}

    check(not differences,
          'main_character_export.FBX_COMMON difiere del preset %s: %s' % (PRESET_NAME, differences))
    return {'checked': True, 'differences': differences}


def build_calibration_scene():
    reset()

    # --- Cubo de 1 m ---
    bpy.ops.mesh.primitive_cube_add(size=CUBE_SIZE_M, location=(0, 0, CUBE_SIZE_M / 2))
    cube = bpy.context.active_object
    cube.name = 'SM_Calibration_Cube_1m'
    check(abs(cube.dimensions.x - CUBE_SIZE_M) < 1e-5,
          'el cubo no mide %s m en X: %s' % (CUBE_SIZE_M, cube.dimensions.x))

    # --- Ejes: cada uno de un largo distinto para poder decir CUÁL se volteó ---
    # Con tres ejes iguales, una permutación de ejes en el FBX es indetectable.
    axes = []
    for name, direction, length in (
        ('X', Vector((1, 0, 0)), AXIS_LENGTH_M),
        ('Y', Vector((0, 1, 0)), AXIS_LENGTH_M * 0.7),
        ('Z', Vector((0, 0, 1)), AXIS_LENGTH_M * 0.4),
    ):
        bpy.ops.mesh.primitive_cube_add(size=0.05, location=tuple(direction * (length / 2)))
        axis = bpy.context.active_object
        axis.name = 'SM_Calibration_Axis_%s' % name
        axis.scale = (
            length / 0.05 if name == 'X' else 1.0,
            length / 0.05 if name == 'Y' else 1.0,
            length / 0.05 if name == 'Z' else 1.0,
        )
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        axes.append(axis)

    # --- Mannequin esquelético de 1,83 m con un hueso y un shape key ---
    bpy.ops.object.armature_add(location=(2.0, 0, 0))
    rig = bpy.context.active_object
    rig.name = 'SKEL_Calibration_Mannequin'
    bpy.ops.object.mode_set(mode='EDIT')
    bone = rig.data.edit_bones[0]
    bone.name = 'root'
    bone.head = (0, 0, 0)
    bone.tail = (0, 0, MANNEQUIN_HEIGHT_M)
    bpy.ops.object.mode_set(mode='OBJECT')

    bpy.ops.mesh.primitive_cylinder_add(radius=0.15, depth=MANNEQUIN_HEIGHT_M,
                                        location=(2.0, 0, MANNEQUIN_HEIGHT_M / 2))
    body = bpy.context.active_object
    body.name = 'SK_Calibration_Mannequin'
    check(abs(body.dimensions.z - MANNEQUIN_HEIGHT_M) < 1e-4,
          'el mannequin no mide %s m: %s' % (MANNEQUIN_HEIGHT_M, body.dimensions.z))

    modifier = body.modifiers.new(name='Armature', type='ARMATURE')
    modifier.object = rig
    group = body.vertex_groups.new(name='root')
    group.add([v.index for v in body.data.vertices], 1.0, 'REPLACE')
    body.parent = rig

    # Shape key: la base y un desplazamiento medible. Si el morph no viaja por FBX, en Unreal la
    # deformación será cero y se ve en el número, no "a ojo".
    body.shape_key_add(name='Basis')
    widen = body.shape_key_add(name='Calibration_Widen')
    for vertex in widen.data:
        vertex.co.x += 0.10
    check(len(body.data.shape_keys.key_blocks) == 2,
          'el mannequin no tiene los dos shape keys esperados')

    # --- Una Action con contacto conocido ---
    rig.animation_data_create()
    action = bpy.data.actions.new(name='AN_Calibration_Rise')
    rig.animation_data.action = action
    bpy.context.scene.frame_start = 1
    bpy.context.scene.frame_end = 31   # 30 frames a 30 FPS = 1,0 s exacto

    # `pose_bone.location` está en espacio LOCAL del hueso, y en Blender el eje local Y corre a
    # lo largo del hueso. Como este hueso apunta hacia arriba, subirlo es mover Y, no Z. Animar Z
    # lo desplazaría de costado, que es lo que hacía la primera versión de este script.
    pose_bone = rig.pose.bones['root']
    for frame, offset_along_bone in ((1, 0.0), (16, 0.5), (31, 0.0)):
        pose_bone.location = (0.0, offset_along_bone, 0.0)
        pose_bone.keyframe_insert(data_path='location', frame=frame)

    # Evaluar el rig en dos frames en vez de contar fcurves. Blender 5.2 quitó `Action.fcurves`
    # —las acciones pasaron a tener slots en 4.4— y contar curvas ataba esta comprobación a una
    # versión concreta del API. Evaluar prueba además lo que de verdad importa: que la animación
    # mueve el hueso, no que existan curvas.
    def bone_translation_at(frame):
        bpy.context.scene.frame_set(frame)
        bpy.context.view_layer.update()
        return rig.pose.bones['root'].matrix.translation.copy()

    rest = bone_translation_at(1)
    peak = bone_translation_at(16)
    displacement = (peak - rest).length
    # Magnitud, no un eje concreto: si el rig cambia de roll, un check por eje se rompería sin
    # que la animación tenga nada malo.
    check(displacement > 0.4,
          'la Action no mueve el hueso: desplazamiento %.3f m entre frame 1 y 16' % displacement)
    bpy.context.scene.frame_set(1)

    return cube, axes, rig, body, action, {
        'rest': list(rest), 'peak': list(peak), 'displacement_m': displacement,
    }


def export(cube, axes, rig, body):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    written = {}

    def select(objects):
        bpy.ops.object.mode_set(mode='OBJECT')
        bpy.ops.object.select_all(action='DESELECT')
        for obj in objects:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = objects[0]

    def run(path, objects, kwargs):
        select(objects)
        result = bpy.ops.export_scene.fbx(filepath=str(path), **kwargs)
        ok = 'FINISHED' in result and path.is_file() and path.stat().st_size > 0
        check(ok, 'fallo la exportacion de %s' % path.name)
        written[path.name] = path.stat().st_size if path.is_file() else 0

    run(OUT_DIR / 'SM_Calibration_Cube_1m.fbx', [cube], static_mesh_kwargs())
    for axis in axes:
        run(OUT_DIR / (axis.name + '.fbx'), [axis], static_mesh_kwargs())
    run(OUT_DIR / 'SK_Calibration_Mannequin.fbx', [rig, body], skeletal_mesh_kwargs())
    run(OUT_DIR / 'AN_Calibration_Rise.fbx', [rig], animation_kwargs())

    return written


def main():
    drift = verify_preset_has_not_drifted()
    cube, axes, rig, body, action, anim_probe = build_calibration_scene()
    written = export(cube, axes, rig, body)

    EVIDENCE_DIR.mkdir(parents=True, exist_ok=True)
    report = {
        'timestamp': datetime.now().isoformat(),
        'blender_version': bpy.app.version_string,
        'blender_build_date': bpy.app.build_date.decode('utf-8') if isinstance(bpy.app.build_date, bytes) else str(bpy.app.build_date),
        'python_version': platform.python_version(),
        'os': platform.platform(),
        'preset': describe(),
        'preset_drift_check': drift,
        'scene': {
            'unit_system': bpy.context.scene.unit_settings.system,
            'unit_scale_length': bpy.context.scene.unit_settings.scale_length,
            'fps': bpy.context.scene.render.fps,
            'cube_size_m': CUBE_SIZE_M,
            'mannequin_height_m': MANNEQUIN_HEIGHT_M,
            'axis_lengths_m': {'X': AXIS_LENGTH_M, 'Y': AXIS_LENGTH_M * 0.7, 'Z': AXIS_LENGTH_M * 0.4},
            'shape_keys': [k.name for k in body.data.shape_keys.key_blocks],
            'action': action.name,
            'action_frames': [bpy.context.scene.frame_start, bpy.context.scene.frame_end],
            'action_bone_z': anim_probe,
        },
        'exported': written,
        'failures': failures,
        'passed': not failures,
    }

    out = EVIDENCE_DIR / ('calibration_blender_%s.json' % datetime.now().strftime('%Y%m%d_%H%M%S'))
    out.write_text(json.dumps(report, indent=2), encoding='utf-8')

    print('CALIBRATION: Blender %s, preset %s v%d' % (bpy.app.version_string, PRESET_NAME, PRESET_VERSION))
    for name, size in written.items():
        print('CALIBRATION: %s (%d bytes)' % (name, size))
    print('CALIBRATION: informe %s' % out)

    if failures:
        for failure in failures:
            print('CALIBRATION: FALLO %s' % failure)
        print('CALIBRATION: RESULTADO=FALLO')
        sys.exit(1)

    print('CALIBRATION: RESULTADO=OK')


if __name__ == '__main__':
    main()
