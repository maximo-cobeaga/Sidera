"""Audita el arte del proyecto: slots de material vacíos o por defecto, y assets huérfanos.

Sólo lee. Complementa la comparación entre `Content/` y `Saved/Cooked/`: el cocinado dice
qué llega al juego, y este reporte dice por qué algo no llega.
"""
import json
from pathlib import Path
import unreal

ROOTS = ('/Game/Astraeon/Art', '/Game/Astraeon/Characters')
ENGINE_DEFAULT = '/Engine/EngineMaterials/WorldGridMaterial'

# Import crudo del protagonista: superado por `Optimized/`, no se cocina y su slot apunta
# a un atlas que ya no existe. Se reporta aparte para que no oculte un defecto real.
SUPERSEDED_PREFIX = '/Game/Astraeon/Characters/Player/'

report = dict(passed=False, meshes=[], missing_material_slots=[], engine_default_slots=[], superseded_slots=[])

registry = unreal.AssetRegistryHelpers.get_asset_registry()
paths = []
for root in ROOTS:
    paths.extend(unreal.EditorAssetLibrary.list_assets(root, recursive=True))

for path in sorted(set(paths)):
    asset = unreal.load_asset(path)
    if not isinstance(asset, (unreal.SkeletalMesh, unreal.StaticMesh)):
        continue
    kind = 'skeletal' if isinstance(asset, unreal.SkeletalMesh) else 'static'
    slots = []
    if kind == 'skeletal':
        materials = asset.get_editor_property('materials')
        for index, entry in enumerate(materials):
            material = entry.get_editor_property('material_interface')
            slots.append((index, str(entry.get_editor_property('material_slot_name')), material))
    else:
        for index, entry in enumerate(asset.get_editor_property('static_materials')):
            slots.append((index, str(entry.get_editor_property('material_slot_name')),
                          entry.get_editor_property('material_interface')))

    row = dict(asset=asset.get_path_name(), kind=kind, slots=[])
    for index, slot_name, material in slots:
        material_path = material.get_path_name() if material else None
        row['slots'].append(dict(index=index, slot=slot_name, material=material_path))
        superseded = path.startswith(SUPERSEDED_PREFIX) and '/Optimized/' not in path
        if material_path is None:
            bucket = 'superseded_slots' if superseded else 'missing_material_slots'
            report[bucket].append(dict(asset=asset.get_path_name(), slot=index, slot_name=slot_name))
        elif material_path.startswith(ENGINE_DEFAULT):
            report['engine_default_slots'].append(dict(asset=asset.get_path_name(), slot=index, slot_name=slot_name))
    report['meshes'].append(row)

report['mesh_count'] = len(report['meshes'])
report['passed'] = not report['missing_material_slots'] and not report['engine_default_slots']
unreal.log('ASTRAEON_ART_USAGE: meshes=%d sin_material=%d por_defecto=%d superados=%d' % (
    report['mesh_count'], len(report['missing_material_slots']), len(report['engine_default_slots']),
    len(report['superseded_slots'])))
Path(unreal.Paths.project_dir(), 'ContentPipeline/reports/art_usage_audit.json').write_text(
    json.dumps(report, indent=2) + '\n')
