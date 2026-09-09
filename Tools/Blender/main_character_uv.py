"""Reconstruye las UV del protagonista para el bake.

Problema que resuelve (auditoria 2026-09-08):
  - `smart_project` partia la cabeza en 68 islas, incluida una costura vertical
    por la linea media de la cara (frente -> nariz -> menton).
  - El traje quedaba en 1001 islas usando solo el 34% del texel.

La region `MAT_Player_Character` es una sola componente conexa con
caracteristica de Euler = 1, es decir un disco topologico: puede desenvolverse
como UNA isla sin ninguna costura interior. La unica frontera es el anillo del
cuello, que queda oculto bajo el traje.
"""
import json
from pathlib import Path
import bpy

ROOT = Path(r'C:/Users/MAXIMO/Desktop/Astraeon')
ASSET = ROOT / 'graphics/characters/main_player'
MESH_NAME = 'SK_Astraeon_Player'


def _activate(obj):
    if bpy.context.object and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    obj.hide_viewport = False
    obj.hide_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def _select_material(me, index):
    for face in me.polygons:
        face.select = face.material_index == index


def _uv_area(me, index):
    """Area total ocupada en UV por las caras de un material."""
    uv = me.uv_layers[0].data
    total = 0.0
    for face in me.polygons:
        if face.material_index != index:
            continue
        pts = [uv[li].uv for li in face.loop_indices]
        acc = 0.0
        for i in range(1, len(pts) - 1):
            a, b, c = pts[0], pts[i], pts[i + 1]
            acc += abs((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y)) / 2
        total += acc
    return total


def _islands(me, index):
    import collections
    uv = me.uv_layers[0].data
    parent = {}

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    def union(a, b):
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[ra] = rb

    loops = [li for f in me.polygons if f.material_index == index for li in f.loop_indices]
    for li in loops:
        parent[li] = li
    buckets = collections.defaultdict(list)
    for li in loops:
        u, v = uv[li].uv
        buckets[(round(u, 5), round(v, 5))].append(li)
    for group in buckets.values():
        for li in group[1:]:
            union(group[0], li)
    for face in me.polygons:
        if face.material_index != index:
            continue
        ls = list(face.loop_indices)
        for li in ls[1:]:
            union(ls[0], li)
    return len({find(li) for li in loops})


def _image_editor_override():
    """Devuelve (override, restore) usando un area temporal de editor UV."""
    window = bpy.context.window_manager.windows[0]
    area = min(window.screen.areas, key=lambda a: a.width * a.height)
    previous = area.type
    area.type = 'IMAGE_EDITOR'
    area.ui_type = 'UV'
    space = area.spaces.active
    override = {'window': window, 'screen': window.screen, 'area': area,
                'space_data': space, 'region': next(r for r in area.regions if r.type == 'WINDOW')}

    def restore():
        area.type = previous
    return override, restore


def rebuild():
    obj = bpy.data.objects[MESH_NAME]
    me = obj.data
    assert me.uv_layers, 'sin capa UV'
    tools = bpy.context.scene.tool_settings
    previous_sync = tools.use_uv_select_sync
    tools.use_uv_select_sync = True
    tools.mesh_select_mode = (False, False, True)
    _activate(obj)

    override, restore = _image_editor_override()
    report = {'status': 'REBUILDING'}
    try:
        # 1. Cabeza: una sola isla, costura unicamente en el anillo del cuello.
        _select_material(me, 0)
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.uv.unwrap(method='ANGLE_BASED', margin=0.001,
                          fill_holes=True, correct_aspect=True)
        with bpy.context.temp_override(**override):
            bpy.ops.uv.pack_islands(rotate=True, rotate_method='ANY', scale=True,
                                    shape_method='CONCAVE', margin_method='FRACTION',
                                    margin=0.004)
        bpy.ops.object.mode_set(mode='OBJECT')

        # 2. Traje: proyeccion por angulo, escala homogenea y empaquetado denso.
        _select_material(me, 1)
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.uv.smart_project(angle_limit=1.15191731, island_margin=0.0,
                                 area_weight=0.3, correct_aspect=True,
                                 scale_to_bounds=False)
        with bpy.context.temp_override(**override):
            bpy.ops.uv.average_islands_scale()
            bpy.ops.uv.pack_islands(rotate=True, rotate_method='ANY', scale=True,
                                    shape_method='CONCAVE', margin_method='FRACTION',
                                    margin=0.0025)
        bpy.ops.object.mode_set(mode='OBJECT')

        report = {
            'status': 'UV_REBUILT',
            'character': {'islands': _islands(me, 0), 'uv_coverage': round(_uv_area(me, 0), 4)},
            'suit': {'islands': _islands(me, 1), 'uv_coverage': round(_uv_area(me, 1), 4)},
        }
    finally:
        if bpy.context.object and bpy.context.object.mode != 'OBJECT':
            bpy.ops.object.mode_set(mode='OBJECT')
        restore()
        tools.use_uv_select_sync = previous_sync
    (ASSET / 'docs/uv_rebuild.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


EQUIPMENT = ('SK_Astraeon_Helmet', 'SK_Astraeon_Backpack', 'SK_Astraeon_WristComputer')


def rebuild_equipment():
    """Desenvuelve el equipo, que se construyo sin ninguna capa UV.

    Sin UV las piezas no admiten textura propia ni bake, y Unreal avisa por
    falta de UV de lightmap al importarlas.
    """
    tools = bpy.context.scene.tool_settings
    previous_sync = tools.use_uv_select_sync
    tools.use_uv_select_sync = True
    tools.mesh_select_mode = (False, False, True)
    override, restore = _image_editor_override()
    report = {'status': 'EQUIPMENT_UV_REBUILT', 'pieces': []}
    try:
        for name in EQUIPMENT:
            obj = bpy.data.objects.get(name)
            if obj is None:
                report['pieces'].append({'name': name, 'status': 'MISSING'})
                continue
            me = obj.data
            if not me.uv_layers:
                me.uv_layers.new(name='UVMap')
            _activate(obj)
            for face in me.polygons:
                face.select = True
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.uv.smart_project(angle_limit=1.15191731, island_margin=0.0,
                                     area_weight=0.0, correct_aspect=True,
                                     scale_to_bounds=False)
            with bpy.context.temp_override(**override):
                bpy.ops.uv.average_islands_scale()
                bpy.ops.uv.pack_islands(rotate=True, rotate_method='ANY', scale=True,
                                        shape_method='CONCAVE',
                                        margin_method='FRACTION', margin=0.01)
            bpy.ops.object.mode_set(mode='OBJECT')
            report['pieces'].append({
                'name': name, 'uv_layers': len(me.uv_layers),
                'islands': _islands(me, 0) if len(me.materials) <= 1 else
                           sum(_islands(me, i) for i in range(len(me.materials))),
                'uv_coverage': round(sum(_uv_area(me, i) for i in range(max(1, len(me.materials)))), 4),
                'faces': len(me.polygons),
            })
    finally:
        if bpy.context.object and bpy.context.object.mode != 'OBJECT':
            bpy.ops.object.mode_set(mode='OBJECT')
        restore()
        tools.use_uv_select_sync = previous_sync
    (ASSET / 'docs/uv_equipment.json').write_text(json.dumps(report, indent=2) + '\n')
    return report
