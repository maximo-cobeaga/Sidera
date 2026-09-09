"""Sets de textura Gear y Helmet para el equipo del protagonista.

`Scripts/Editor/MainCharacterAppearance.py` construye materiales para cuatro
regiones — Character, Suit, Gear y Helmet — y exige
`T_Player_<region>_{Color,NormalGL,ORM}.png`. Solo existian Character y Suit,
asi que la validacion de Unreal no podia pasar.

El equipo se construyo con cinco materiales procedurales planos
(Ceramic/Graphite/Bronze/Visor/Display). Aqui se hornean a un set por region:

    Helmet  <- SK_Astraeon_Helmet
    Gear    <- SK_Astraeon_Backpack + SK_Astraeon_WristComputer

Como Gear comparte una sola imagen entre dos objetos, primero se reparten sus UV
en mitades disjuntas del espacio 0-1; si no, el segundo horneado pisaria al
primero.

El ORM no se puede hornear directo: Blender no tiene un pase de metalico. Se
sustituye temporalmente la superficie de cada material por una emision de color
(1, roughness, metallic) y se hornea EMIT, lo que da el canal exacto.
"""
import json
from pathlib import Path
import bpy
import numpy as np

ROOT = Path(r'C:/Users/MAXIMO/Desktop/Astraeon')
ASSET = ROOT / 'graphics/characters/main_player'
SIZE = 1024
MARGIN = 12

REGIONS = {
    'Helmet': ['SK_Astraeon_Helmet'],
    'Gear': ['SK_Astraeon_Backpack', 'SK_Astraeon_WristComputer'],
}
SMOOTH_ANGLE_DEG = 35.0


def _activate(obj):
    if bpy.context.object and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    obj.hide_viewport = False
    obj.hide_render = False      # sin esto el bake rechaza el objeto
    obj.hide_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def _read(image):
    buf = np.empty(len(image.pixels), dtype=np.float32)
    image.pixels.foreach_get(buf)
    return buf.reshape(image.size[1], image.size[0], 4)


def _write(image, array):
    image.pixels.foreach_set(array.reshape(-1).astype(np.float32))
    image.update()


def split_gear_uvs():
    """Reparte las UV de mochila y computadora en mitades disjuntas de U."""
    layout = {'SK_Astraeon_Backpack': (0.0, 0.5), 'SK_Astraeon_WristComputer': (0.5, 1.0)}
    done = {}
    for name, (u0, u1) in layout.items():
        obj = bpy.data.objects[name]
        uv = obj.data.uv_layers[0].data
        if obj.data.get('gear_atlas_slot') == name:
            done[name] = 'ya repartido'
            continue
        for loop in uv:
            loop.uv[0] = u0 + loop.uv[0] * (u1 - u0)
        obj.data['gear_atlas_slot'] = name
        done[name] = [u0, u1]
    return done


def smooth_shading(angle_deg=SMOOTH_ANGLE_DEG):
    """Sombreado suave con division por angulo: quita el facetado del casco."""
    out = {}
    for names in REGIONS.values():
        for name in names:
            obj = bpy.data.objects[name]
            _activate(obj)
            for polygon in obj.data.polygons:
                polygon.use_smooth = True
            try:
                bpy.ops.object.shade_smooth_by_angle(angle=np.radians(angle_deg))
                out[name] = 'shade_smooth_by_angle %.0f' % angle_deg
            except Exception as error:
                out[name] = 'solo use_smooth (%s)' % type(error).__name__
    return out


def _targets(objects, image):
    for obj in objects:
        for material in obj.data.materials:
            material.use_nodes = True
            nodes = material.node_tree.nodes
            node = nodes.get('BAKE_TARGET') or nodes.new('ShaderNodeTexImage')
            node.name = 'BAKE_TARGET'
            node.image = image
            nodes.active = node


def _orm_override(objects):
    """Cambia cada material a una emision (1, roughness, metallic). Devuelve el deshacer."""
    undo = []
    for obj in objects:
        for material in obj.data.materials:
            tree = material.node_tree
            output = next(n for n in tree.nodes if n.type == 'OUTPUT_MATERIAL')
            link = output.inputs['Surface'].links[0]
            original = link.from_socket
            bsdf = next(n for n in tree.nodes if n.type == 'BSDF_PRINCIPLED')
            roughness = bsdf.inputs['Roughness'].default_value
            metallic = bsdf.inputs['Metallic'].default_value
            emission = tree.nodes.new('ShaderNodeEmission')
            emission.name = 'ORM_TEMP'
            emission.inputs['Color'].default_value = (1.0, roughness, metallic, 1.0)
            tree.links.new(emission.outputs[0], output.inputs['Surface'])
            undo.append((tree, output, original, emission))
    return undo


def _restore(undo):
    for tree, output, original, emission in undo:
        tree.links.new(original, output.inputs['Surface'])
        tree.nodes.remove(emission)


def _dilate(images, iterations=20):
    color = _read(images['Color'])
    hole = np.all(color[:, :, :3] < 0.002, axis=2)
    caches = {k: _read(v) for k, v in images.items()}
    filled = 0
    for _ in range(iterations):
        if not hole.any():
            break
        valid = (~hole).astype(np.float32)
        neighbours = sum(np.roll(valid, s, axis=a) for s, a in ((1, 0), (-1, 0), (1, 1), (-1, 1)))
        target = hole & (neighbours > 0)
        if not target.any():
            break
        for data in caches.values():
            acc = sum(np.roll(data[:, :, :3] * valid[:, :, None], s, axis=a)
                      for s, a in ((1, 0), (-1, 0), (1, 1), (-1, 1)))
            data[:, :, :3] = np.where(target[:, :, None],
                                      acc / np.maximum(neighbours, 1)[:, :, None],
                                      data[:, :, :3])
        filled += int(target.sum())
        hole = hole & ~target
    for kind, data in caches.items():
        _write(images[kind], data)
    return filled


def bake():
    scene = bpy.context.scene
    engine = scene.render.engine
    report = {'status': 'EQUIPMENT_BAKED', 'size': SIZE,
              'gear_uv_split': split_gear_uvs(), 'smooth': smooth_shading(), 'regions': {}}
    try:
        scene.render.engine = 'CYCLES'
        scene.cycles.samples = 8
        scene.cycles.device = 'CPU'
        for region, names in REGIONS.items():
            objects = [bpy.data.objects[n] for n in names]
            images = {}
            for kind in ('Color', 'NormalGL', 'ORM', 'Emission'):
                name = 'T_Player_%s_%s' % (region, kind)
                image = bpy.data.images.get(name) or bpy.data.images.new(
                    name, width=SIZE, height=SIZE, alpha=False)
                image.colorspace_settings.name = 'sRGB' if kind == 'Color' else 'Non-Color'
                images[kind] = image

            for kind in ('Color', 'NormalGL', 'ORM', 'Emission'):
                _targets(objects, images[kind])
                undo = _orm_override(objects) if kind == 'ORM' else []
                try:
                    for index, obj in enumerate(objects):
                        _activate(obj)
                        kwargs = dict(type={'Color': 'DIFFUSE', 'NormalGL': 'NORMAL',
                                            'ORM': 'EMIT', 'Emission': 'EMIT'}[kind],
                                      use_selected_to_active=False, margin=MARGIN,
                                      margin_type='ADJACENT_FACES', use_clear=index == 0)
                        if kind == 'Color':
                            kwargs['pass_filter'] = {'COLOR'}
                        if kind == 'NormalGL':
                            kwargs.update(normal_space='TANGENT', normal_g='POS_Y')
                        assert 'FINISHED' in bpy.ops.object.bake(**kwargs), kind
                finally:
                    _restore(undo)

            filled = _dilate(images)
            saved = []
            for kind, image in images.items():
                image.filepath_raw = str(ASSET / 'textures' / ('%s.png' % image.name))
                image.file_format = 'PNG'
                image.save()
                saved.append(image.name)
            report['regions'][region] = {'objects': names, 'dilated_texels': filled,
                                         'images': saved}
    finally:
        scene.render.engine = engine
        (ASSET / 'docs/equipment_bake.json').write_text(json.dumps(report, indent=2) + '\n')
    return report
