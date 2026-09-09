"""Regenera LOD1-LOD3 del protagonista desde el LOD0 optimizado.

Auditoria 2026-09-08: los LOD en el archivo seguian derivados de la malla
antigua de 233k triangulos. LOD1 tenia 93.538 triangulos, MAS que el nuevo
LOD0 de 65.284, y los tres usaban `MAT_Astraeon_Player_Atlas` mientras LOD0
ya usa el par Character/Suit. La cadena de detalle estaba invertida.

Los LOD se derivan por colapso desde LOD0, de modo que heredan UV, materiales
y pesos de los 75 huesos sin volver a transferir nada.
"""
import json
from pathlib import Path
import bpy

ROOT = Path(r'C:/Users/MAXIMO/Desktop/Astraeon')
ASSET = ROOT / 'graphics/characters/main_player'
MESH_NAME = 'SK_Astraeon_Player'
RIG_NAME = 'SKEL_Astraeon_Player'

# Proporcion de triangulos respecto de LOD0 (65.284 tris).
RATIOS = {1: 0.50, 2: 0.25, 3: 0.10}


def _triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def _archive(name):
    """Aparta un LOD viejo en lugar de borrarlo, para poder volver atras."""
    old = bpy.data.objects.get(name)
    if old is None:
        return None
    archived = 'ARCHIVE_%s_Legacy' % name
    existing = bpy.data.objects.get(archived)
    if existing is not None:
        return existing.name
    old.name = archived
    old.data.name = archived
    old.hide_render = True
    old.hide_set(True)
    return archived


def rebuild():
    source = bpy.data.objects[MESH_NAME]
    rig = bpy.data.objects[RIG_NAME]
    base_tris = _triangles(source)
    report = {'status': 'LOD_REBUILT', 'lod0_triangles': base_tris,
              'materials': [m.name for m in source.data.materials], 'levels': [],
              'archived': []}

    for level, ratio in sorted(RATIOS.items()):
        name = '%s_LOD%d' % (MESH_NAME, level)
        archived = _archive(name)
        if archived:
            report['archived'].append(archived)

        mesh = source.data.copy()
        mesh.name = name
        lod = bpy.data.objects.new(name, mesh)
        for collection in source.users_collection:
            collection.objects.link(lod)
        lod.matrix_world = source.matrix_world.copy()

        decimate = lod.modifiers.new('LOD_Collapse', 'DECIMATE')
        decimate.decimate_type = 'COLLAPSE'
        decimate.ratio = ratio
        decimate.use_collapse_triangulate = True
        # Los pesos deben sobrevivir el colapso para conservar el skinning.
        decimate.vertex_group_factor = 1.0

        bpy.context.view_layer.objects.active = lod
        bpy.ops.object.modifier_apply(modifier=decimate.name)

        lod.parent = rig
        lod.matrix_parent_inverse = rig.matrix_world.inverted()
        skin = lod.modifiers.new('ARM_Skin', 'ARMATURE')
        skin.object = rig
        lod.hide_render = True
        lod.hide_set(True)

        unweighted = sum(not v.groups for v in lod.data.vertices)
        report['levels'].append({
            'name': name, 'ratio': ratio,
            'triangles': _triangles(lod), 'vertices': len(lod.data.vertices),
            'materials': [m.name for m in lod.data.materials],
            'uv_layers': len(lod.data.uv_layers),
            'vertex_groups': len(lod.vertex_groups),
            'unweighted_vertices': unweighted,
        })

    report['monotonic'] = (
        base_tris > report['levels'][0]['triangles']
        > report['levels'][1]['triangles'] > report['levels'][2]['triangles'])
    (ASSET / 'docs/lod_rebuild.json').write_text(json.dumps(report, indent=2) + '\n')
    return report
