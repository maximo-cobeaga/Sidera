"""Preset de exportación FBX congelado del proyecto — `UE57_AST_V1`.

Fuente única de los ajustes de exportación Blender → Unreal 5.7.4, según
`Docs/ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` §8.13. Congelado en la Fase 0
(ADR 0004) el 2026-09-09.

**Por qué existe.** Estos mismos ajustes vivían duplicados en
`Tools/Blender/exporters/fbx.py` y en `Tools/Blender/main_character_export.py`. Coincidían, pero
nada lo garantizaba: cambiar uno y no el otro produce assets con ejes o escala distintos según
por qué exportador pasaron, y ese defecto no se ve hasta que un personaje aparece rotado 90° o
a 1/100 de su tamaño en Unreal. Ya se pagó una vez esa clase de error —ver el ADR de
normalización de escala de raíz en `Docs/DECISIONS.md`—.

**No modificar sin ADR.** Un preset de exportación es un contrato con todos los assets ya
importados: cambiarlo invalida en silencio los que se exportaron con el anterior.

Uso:
    from presets.UE57_AST_V1 import static_mesh_kwargs, skeletal_mesh_kwargs, animation_kwargs
    bpy.ops.export_scene.fbx(filepath=ruta, **static_mesh_kwargs())
"""

# Versión del preset. Sube sólo con un ADR que explique qué assets hay que reexportar.
PRESET_NAME = "UE57_AST_V1"
PRESET_VERSION = 1

# Ajustes comunes a todo lo que exporta el proyecto.
#
# axis_forward='-Y' / axis_up='Z' y global_scale=1.0 con apply_unit_scale son lo que hace que un
# metro de Blender llegue como 100 cm de Unreal sin tocar "Import Uniform Scale" al importar.
# add_leaf_bones=False evita los huesos hoja que Unreal no usa y que ensucian el esqueleto.
_COMMON = dict(
    use_selection=True,
    global_scale=1.0,
    apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_NONE',
    axis_forward='-Y',
    axis_up='Z',
    add_leaf_bones=False,
    use_mesh_modifiers=False,
    mesh_smooth_type='FACE',
    path_mode='AUTO',
)

# Orientación de huesos. Sólo aplica a esqueléticos, pero se declara aparte para que quede
# explícito que un static mesh NO debe llevarlos.
_BONE_AXES = dict(
    primary_bone_axis='Y',
    secondary_bone_axis='X',
)


def static_mesh_kwargs(**overrides):
    """Malla estática. Sin animación y sin ejes de hueso."""
    kwargs = dict(_COMMON)
    kwargs.update(object_types={'MESH'}, bake_anim=False)
    kwargs.update(overrides)
    return kwargs


def skeletal_mesh_kwargs(**overrides):
    """Malla esquelética en bind pose. Sin animación: los clips van aparte."""
    kwargs = dict(_COMMON)
    kwargs.update(_BONE_AXES)
    kwargs.update(object_types={'ARMATURE', 'MESH'}, bake_anim=False)
    kwargs.update(overrides)
    return kwargs


def animation_kwargs(**overrides):
    """Clips de animación.

    `bake_anim_simplify_factor=0.0` es obligatorio (§8.7 del documento de transición): cualquier
    valor mayor descarta keyframes y el pie que estaba en contacto con el suelo empieza a
    patinar. `bake_anim_step=1.0` conserva un key por frame.
    """
    kwargs = dict(_COMMON)
    kwargs.update(_BONE_AXES)
    kwargs.update(
        object_types={'ARMATURE'},
        bake_anim=True,
        bake_anim_use_all_bones=True,
        bake_anim_use_nla_strips=False,
        bake_anim_use_all_actions=True,
        bake_anim_force_startend_keying=True,
        bake_anim_step=1.0,
        bake_anim_simplify_factor=0.0,
    )
    kwargs.update(overrides)
    return kwargs


def describe():
    """Resumen legible para los informes de evidencia."""
    return {
        'preset': PRESET_NAME,
        'version': PRESET_VERSION,
        'axis_forward': _COMMON['axis_forward'],
        'axis_up': _COMMON['axis_up'],
        'global_scale': _COMMON['global_scale'],
        'apply_unit_scale': _COMMON['apply_unit_scale'],
        'add_leaf_bones': _COMMON['add_leaf_bones'],
        'primary_bone_axis': _BONE_AXES['primary_bone_axis'],
        'secondary_bone_axis': _BONE_AXES['secondary_bone_axis'],
        'animation_simplify_factor': 0.0,
    }
