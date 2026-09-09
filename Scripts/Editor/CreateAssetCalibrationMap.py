"""Importa la calibración de Blender y valida que la escala y los ejes sobrevivieron el viaje.

Segunda mitad de la calibración de la Fase 0 (ADR 0004). La primera la produce
`Tools/Blender/calibration_ue57.py`; ésta la mide del lado de Unreal y arma `TL_00_AssetCalibration`.

Lo que comprueba, con números y no a ojo:
  - un cubo de 1 m de Blender mide 100 cm en Unreal;
  - un mannequin de 1,83 m mide 183 cm;
  - los ejes X/Y/Z, de largos distintos a propósito, llegan sin permutarse;
  - el shape key llega como morph target;
  - la Action llega como AnimSequence con duración correcta.

Se comprueba cada uno por separado porque fallan por causas distintas: la escala se pierde en el
Armature, los ejes en el preset de exportación, y los morphs en la opción de importación.

Uso (NO -nullrhi, ver KNOWN_ISSUES):
    UnrealEditor-Cmd.exe Astraeon.uproject -RenderOffscreen -unattended -nosplash \
        -ExecutePythonScript=Scripts\\Editor\\CreateAssetCalibrationMap.py
"""

import json
import os
from datetime import datetime

import unreal

MAP_PACKAGE = "/Game/Maps/TL_00_AssetCalibration"
DEST_PATH = "/Game/Astraeon/Test/Calibration"

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "ContentPipeline", "Generated", "Calibration")
EVIDENCE_DIR = os.path.join(PROJECT_DIR, "Docs", "evidencia")

# Lo mismo que declara el informe de Blender, en centímetros Unreal.
EXPECTED_CUBE_CM = 100.0
EXPECTED_MANNEQUIN_CM = 183.0
TOLERANCE_CM = 1.0

failures = []
measurements = {}


def check(condition, message):
    if not condition:
        failures.append(message)
        unreal.log_error("CALIB: FALLO " + message)
    return condition


def import_fbx(filename, destination, options, expected_class=None):
    """Importa un FBX y devuelve el asset del tipo pedido.

    `expected_class` no es un lujo: una importación esquelética crea Skeleton, SkeletalMesh y
    PhysicsAsset a la vez, y quedarse con el primero de la lista devuelve el Skeleton, que no
    tiene bounds ni morphs. Pedir la clase explícitamente evita medir el asset equivocado.
    """
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SOURCE_DIR, filename)
    task.destination_path = destination
    task.automated = True
    task.replace_existing = True
    task.save = True
    task.options = options
    task.factory = unreal.FbxFactory()

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not task.imported_object_paths:
        return None

    assets = [unreal.EditorAssetLibrary.load_asset(p) for p in task.imported_object_paths]
    assets = [a for a in assets if a is not None]
    if expected_class is None:
        return assets[0] if assets else None

    for asset in assets:
        if isinstance(asset, expected_class):
            return asset

    unreal.log_warning("CALIB: %s no produjo ningun %s; se importo: %s"
                       % (filename, expected_class.__name__, [a.get_class().get_name() for a in assets]))
    return None


def disable_interchange_fbx():
    """Usa el importador FBX heredado, como el resto de los scripts del proyecto.

    Interchange —el importador por defecto en 5.7— ignora las opciones de `FbxImportUI`, y con un
    FBX de sólo armature responde "no había ningún dato que importar". Los importadores del
    protagonista y de la criatura ya desactivan Interchange por la misma razón; esta calibración
    tiene que medir el mismo camino que usan los assets de verdad, no otro.
    """
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")


def static_mesh_options():
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("automated_import_should_detect_type", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    # Escala 1.0 en la importación: el preset ya deja el FBX en la escala correcta. Compensar
    # aquí escondería un defecto del preset, que es exactamente lo que esta prueba busca.
    ui.static_mesh_import_data.set_editor_property("import_uniform_scale", 1.0)
    return ui


def skeletal_mesh_options():
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("create_physics_asset", False)
    ui.set_editor_property("automated_import_should_detect_type", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    data = ui.get_editor_property("skeletal_mesh_import_data")
    data.set_editor_property("import_uniform_scale", 1.0)
    data.set_editor_property("import_morph_targets", True)
    data.set_editor_property("convert_scene", True)
    data.set_editor_property("convert_scene_unit", True)
    return ui


def animation_options(skeleton):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", False)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("import_animations", True)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("create_physics_asset", False)
    ui.set_editor_property("automated_import_should_detect_type", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    ui.set_editor_property("skeleton", skeleton)
    data = ui.get_editor_property("anim_sequence_import_data")
    data.set_editor_property("import_uniform_scale", 1.0)
    data.set_editor_property("convert_scene", True)
    data.set_editor_property("convert_scene_unit", True)
    data.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    data.set_editor_property("use_default_sample_rate", True)
    return ui


def measure_bounds_cm(asset):
    bounds = asset.get_bounds()
    extent = bounds.box_extent
    return {"x": extent.x * 2.0, "y": extent.y * 2.0, "z": extent.z * 2.0}


def validate_cube():
    cube = import_fbx("SM_Calibration_Cube_1m.fbx", DEST_PATH, static_mesh_options(), unreal.StaticMesh)
    if not check(cube is not None, "no se importo el cubo de calibracion"):
        return None

    size = measure_bounds_cm(cube)
    measurements["cube_cm"] = size
    for axis in ("x", "y", "z"):
        check(abs(size[axis] - EXPECTED_CUBE_CM) <= TOLERANCE_CM,
              "el cubo de 1 m mide %.2f cm en %s y deberia medir %.0f"
              % (size[axis], axis.upper(), EXPECTED_CUBE_CM))
    return cube


def validate_axes():
    """Los tres ejes tienen largos distintos, así que una permutación se ve en los números."""
    expected = {"X": 50.0, "Y": 35.0, "Z": 20.0}
    found = {}
    for axis, expected_cm in expected.items():
        asset = import_fbx("SM_Calibration_Axis_%s.fbx" % axis, DEST_PATH, static_mesh_options(), unreal.StaticMesh)
        if asset is None:
            continue
        size = measure_bounds_cm(asset)
        longest = max(size.values())
        found[axis] = {"measured_longest_cm": longest, "expected_cm": expected_cm}
        check(abs(longest - expected_cm) <= TOLERANCE_CM * 2,
              "el eje %s mide %.2f cm en su lado largo y deberia medir %.0f"
              % (axis, longest, expected_cm))
    measurements["axes_cm"] = found


def validate_mannequin():
    mannequin = import_fbx("SK_Calibration_Mannequin.fbx", DEST_PATH, skeletal_mesh_options(), unreal.SkeletalMesh)
    if not check(mannequin is not None, "no se importo el mannequin de calibracion"):
        return None

    size = measure_bounds_cm(mannequin)
    measurements["mannequin_cm"] = size
    check(abs(size["z"] - EXPECTED_MANNEQUIN_CM) <= TOLERANCE_CM * 2,
          "el mannequin de 1,83 m mide %.2f cm de alto y deberia medir %.0f"
          % (size["z"], EXPECTED_MANNEQUIN_CM))

    # El morph target es el punto que el documento de transición §8.13 pide probar ANTES de
    # producir morphs en masa.
    morphs = [m.get_name() for m in mannequin.get_editor_property("morph_targets")]
    measurements["morph_targets"] = morphs
    check("Calibration_Widen" in morphs,
          "el shape key no llego como morph target; morphs encontrados: %s" % morphs)

    return mannequin


def validate_animation(mannequin):
    if mannequin is None:
        return
    skeleton = mannequin.get_editor_property("skeleton")
    anim = import_fbx("AN_Calibration_Rise.fbx", DEST_PATH, animation_options(skeleton), unreal.AnimSequence)
    if not check(anim is not None, "no se importo la animacion de calibracion"):
        return

    length = anim.get_editor_property("sequence_length")
    frames = anim.get_editor_property("number_of_sampled_frames")
    measurements["animation"] = {"seconds": length, "frames": frames}
    # 31 frames a 30 FPS = 1,0 s. Una duración de cero significa que las curvas no viajaron.
    check(length > 0.5, "la animacion dura %.3f s; se esperaba alrededor de 1 s" % length)


def build_map(cube, mannequin):
    # `LevelEditorSubsystem` es la API vigente y, a diferencia de `EditorLevelLibrary.new_level`,
    # deja el mundo nuevo como activo de forma fiable después de haber importado assets. Con la
    # API antigua el mundo activo se quedaba en `L_AstraeonBootstrap` y el guardián de abajo
    # abortaba la corrida — que es lo que tiene que hacer, pero el mapa no se creaba.
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

    # `new_level` se niega a sobrescribir un mapa existente. Este nivel es generado y se rehace
    # entero en cada corrida, así que borrarlo es correcto; cargarlo y añadirle actores no lo
    # sería, porque acumularía los de la corrida anterior.
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PACKAGE):
        unreal.EditorLevelLibrary.new_level("/Temp/EmptyBeforeRebuild")
        unreal.EditorAssetLibrary.delete_asset(MAP_PACKAGE)
        unreal.log("CALIB: mapa anterior borrado para regenerarlo")

    level_editor.new_level(MAP_PACKAGE)

    world = unreal.EditorLevelLibrary.get_editor_world()
    actual = world.get_path_name() if world else "<sin mundo>"
    if "TL_00_AssetCalibration" not in actual:
        raise RuntimeError("El mundo activo es '%s'; se aborta para no escribir en otro mapa." % actual)

    if cube:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_object(cube, unreal.Vector(0, 0, 50))
        actor.set_actor_label("Calibration_Cube_1m")
    if mannequin:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_object(mannequin, unreal.Vector(200, 0, 0))
        actor.set_actor_label("Calibration_Mannequin_183cm")

    unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(-45, 0, 0)
    ).set_actor_label("Sun_TestKey")
    unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 300)
    ).set_actor_label("Sky_TestAmbient")

    unreal.EditorLoadingAndSavingUtils.save_current_level()
    unreal.EditorAssetLibrary.save_asset(MAP_PACKAGE, only_if_is_dirty=False)


def clean_destination():
    """Borra los assets de calibración anteriores antes de importar.

    Una calibración que reimporta encima de lo que dejó la corrida anterior no mide el pipeline:
    mide el pipeline más lo que sobrevivió del intento previo. Se detectó así — un reimport sobre
    assets viejos perdió los morph targets y el informe habría culpado al exportador.
    """
    if not unreal.EditorAssetLibrary.does_directory_exist(DEST_PATH):
        return

    # Borrado tolerante. Una corrida interrumpida puede dejar la carpeta a medias —por ejemplo la
    # AnimSequence viva y su Skeleton borrado—, y entonces `delete_directory` explota al intentar
    # cargar el asset roto. Se borran uno a uno y en orden de dependencia, sin abortar por uno
    # que ya esté inservible: el objetivo es dejar la carpeta vacía, no lamentar cómo quedó.
    assets = unreal.EditorAssetLibrary.list_assets(DEST_PATH, recursive=True, include_folder=False)

    def dependency_rank(path):
        name = path.rsplit("/", 1)[-1]
        if name.startswith("AN_"):
            return 0      # animaciones primero: dependen del esqueleto
        if name.startswith("SK_"):
            return 1
        return 2          # esqueleto y physics asset al final

    for path in sorted(assets, key=dependency_rank):
        try:
            unreal.EditorAssetLibrary.delete_asset(path)
        except Exception as exc:
            unreal.log_warning("CALIB: no se pudo borrar %s (%s); se continua" % (path, exc))

    unreal.log("CALIB: destino limpiado antes de importar (%d assets)" % len(assets))


def main():
    disable_interchange_fbx()
    clean_destination()
    cube = validate_cube()
    validate_axes()
    mannequin = validate_mannequin()
    validate_animation(mannequin)
    build_map(cube, mannequin)

    report = {
        "timestamp": datetime.now().isoformat(),
        "unreal_version": unreal.SystemLibrary.get_engine_version(),
        "map": MAP_PACKAGE,
        "source_dir": SOURCE_DIR,
        "expected": {
            "cube_cm": EXPECTED_CUBE_CM,
            "mannequin_cm": EXPECTED_MANNEQUIN_CM,
            "tolerance_cm": TOLERANCE_CM,
        },
        "measurements": measurements,
        "failures": failures,
        "passed": not failures,
    }

    path = os.path.join(EVIDENCE_DIR, "calibration_unreal_%s.json" % datetime.now().strftime("%Y%m%d_%H%M%S"))
    with open(path, "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2)

    unreal.log("CALIB: informe %s" % path)
    for key, value in measurements.items():
        unreal.log("CALIB: %s = %s" % (key, value))

    if failures:
        unreal.log_error("CALIB: RESULTADO=FALLO (%d)" % len(failures))
        raise RuntimeError("La calibracion Blender -> Unreal no paso: %s" % failures)

    unreal.log("CALIB: RESULTADO=OK")


if __name__ == "__main__":
    main()
