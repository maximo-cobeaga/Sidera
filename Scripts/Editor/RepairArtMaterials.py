"""Vuelve a enganchar los materiales generados a los slots que quedaron en nulo.

El FBX trae el nombre de material de Blender como nombre de slot, y los scripts de
preparación crean un material con ese mismo nombre en la misma carpeta. Cuando un slot
queda vacío el motor dibuja el material por defecto: el asset existe, está cocinado, y aun
así no se ve como se autorizó.

Por qué se construye la lista entera en vez de tocar los slots existentes: mutar los structs
que devuelve `get_editor_property('materials')` opera sobre copias, así que la asignación
parecía funcionar —el script no fallaba, el reporte decía "reparado"— y el paquete se
guardaba sin ella. Ese es el defecto original de los scripts de preparación. Por eso aquí se
**vuelve a leer después de guardar** y se marca cada slot como verificado o no: un reporte
que no comprueba lo que afirma es cómo se llegó hasta aquí.

`/Game/Astraeon/Characters/Player` (import crudo, sin `Optimized`) queda fuera a propósito:
está superado por la versión optimizada y su slot apunta al atlas viejo, que ya no existe.
"""
import json
from pathlib import Path
import unreal

ROOTS = ('/Game/Astraeon/Art', '/Game/Astraeon/Characters/Player/Optimized')
report = dict(passed=False, repaired=[], unresolved=[], unverified=[])


def slot_entries(asset, is_skeletal):
    return list(asset.get_editor_property('materials' if is_skeletal else 'static_materials'))


for root in ROOTS:
    for path in sorted(unreal.EditorAssetLibrary.list_assets(root, recursive=True)):
        asset = unreal.load_asset(path)
        if not isinstance(asset, (unreal.SkeletalMesh, unreal.StaticMesh)):
            continue
        is_skeletal = isinstance(asset, unreal.SkeletalMesh)
        property_name = 'materials' if is_skeletal else 'static_materials'
        folder = path.rsplit('/', 1)[0]

        rebuilt = []
        changed = []
        for index, slot in enumerate(slot_entries(asset, is_skeletal)):
            slot_name = str(slot.get_editor_property('material_slot_name'))
            material = slot.get_editor_property('material_interface')
            if material is None:
                candidate = folder + '/' + slot_name
                if unreal.EditorAssetLibrary.does_asset_exist(candidate):
                    material = unreal.load_asset(candidate)
                    changed.append(dict(asset=path, slot=index, slot_name=slot_name, material=candidate))
                else:
                    report['unresolved'].append(dict(asset=path, slot=index, slot_name=slot_name))
            entry = unreal.SkeletalMaterial() if is_skeletal else unreal.StaticMaterial()
            entry.set_editor_property('material_interface', material)
            entry.set_editor_property('material_slot_name', slot_name)
            rebuilt.append(entry)

        if not changed:
            continue

        asset.modify()
        asset.set_editor_property(property_name, rebuilt)
        assert unreal.EditorAssetLibrary.save_loaded_asset(asset), 'No se pudo guardar ' + path

        # Comprobación en el asset ya guardado, no en la lista que acabamos de escribir.
        written = slot_entries(asset, is_skeletal)
        for entry in changed:
            actual = written[entry['slot']].get_editor_property('material_interface')
            if actual is None:
                report['unverified'].append(entry)
            else:
                entry['written'] = actual.get_path_name()
                report['repaired'].append(entry)

report['passed'] = not report['unresolved'] and not report['unverified']
unreal.log('ASTRAEON_ART_MATERIALS: reparados=%d sin_verificar=%d sin_resolver=%d' % (
    len(report['repaired']), len(report['unverified']), len(report['unresolved'])))
Path(unreal.Paths.project_dir(), 'ContentPipeline/reports/art_material_repair.json').write_text(
    json.dumps(report, indent=2) + '\n')
assert not report['unverified'], 'Slots que no persistieron: %d' % len(report['unverified'])
