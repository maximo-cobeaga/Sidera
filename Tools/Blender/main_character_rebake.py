"""Re-bake del protagonista sobre las UV reconstruidas + relleno de huecos.

Corrige los dos defectos vistos en QA_Retopo_Face_Baked.png:
  1. Costura vertical en la linea media de la cara -> resuelta en
     `main_character_uv.rebuild()`, que deja la cabeza en UNA sola isla.
  2. Huecos negros dentro de las islas del pelo: rayos del bake que atraviesan
     los huecos entre mechones y no impactan nada. Se atacan por dos vias:
       - cage mayor, para que el rayo arranque dentro del volumen del pelo y
         encuentre el mechon o el cuero cabelludo en lugar de vacio;
       - dilatacion iterativa posterior de los texels no horneados, que ademas
         amplia el sangrado del borde de isla.
"""
import json
from pathlib import Path
import time
import traceback
import bpy
import numpy as np

ROOT = Path(r'C:/Users/MAXIMO/Desktop/Astraeon')
ASSET = ROOT / 'graphics/characters/main_player'
REPORT = ASSET / 'docs/bake_progress.json'

CAGE_EXTRUSION = 0.008
MAX_RAY_DISTANCE = 0.030
MARGIN = 16
INPAINT_ITERATIONS = 24


def _activate(obj):
    if bpy.context.object and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    obj.hide_viewport = False
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


def inpaint(images, iterations=INPAINT_ITERATIONS):
    """Rellena los texels no horneados propagando los vecinos validos.

    La mascara de huecos se calcula UNA vez sobre el mapa Color (un texel sin
    impacto queda en negro puro) y se aplica igual a todos los mapas, porque
    comparten UV y la misma tirada de rayos.
    """
    color = _read(images['Color'])
    hole = np.all(color[:, :, :3] < 0.002, axis=2)
    before = int(hole.sum())
    filled_total = 0
    caches = {kind: _read(img) for kind, img in images.items()}
    for _ in range(iterations):
        if not hole.any():
            break
        valid = (~hole).astype(np.float32)
        neighbours = np.zeros_like(valid)
        for shift, axis in ((1, 0), (-1, 0), (1, 1), (-1, 1)):
            neighbours += np.roll(valid, shift, axis=axis)
        target = hole & (neighbours > 0)
        if not target.any():
            break
        for kind, data in caches.items():
            acc = np.zeros_like(data[:, :, :3])
            for shift, axis in ((1, 0), (-1, 0), (1, 1), (-1, 1)):
                acc += np.roll(data[:, :, :3] * valid[:, :, None], shift, axis=axis)
            data[:, :, :3] = np.where(target[:, :, None],
                                      acc / np.maximum(neighbours, 1)[:, :, None],
                                      data[:, :, :3])
        filled_total += int(target.sum())
        hole = hole & ~target
    for kind, data in caches.items():
        _write(images[kind], data)
    return {'holes_before': before, 'holes_filled': filled_total,
            'holes_remaining': int(hole.sum())}


def rebake():
    start = time.time()
    report = {'status': 'BAKING', 'completed': [],
              'cage_extrusion': CAGE_EXTRUSION, 'max_ray_distance': MAX_RAY_DISTANCE,
              'margin': MARGIN}
    REPORT.write_text(json.dumps(report, indent=2))
    scene = bpy.context.scene
    engine = scene.render.engine
    src = bpy.data.objects['SRC_Player_MESH_01']
    obj = bpy.data.objects['SK_Astraeon_Player']
    old_mats = list(src.data.materials)
    visibility = (src.hide_viewport, src.hide_render, src.hide_get())
    mod = obj.modifiers.get('ARM_Skin')
    mod.show_viewport = False
    mod.show_render = False
    temporary = None
    try:
        scene.render.engine = 'CYCLES'
        scene.cycles.samples = 16
        scene.cycles.device = 'CPU'
        temporary = old_mats[0].copy()
        temporary.name = 'BAKE_Source_Player_Temporary'
        src.data.materials.clear()
        src.data.materials.append(temporary)
        nt = temporary.node_tree
        output = next(n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL')
        original_surface = output.inputs['Surface'].links[0].from_socket
        emission = nt.nodes.new('ShaderNodeEmission')
        orm = next(n for n in nt.nodes if n.type == 'TEX_IMAGE' and n.image
                   and n.image.name.startswith('ORM'))
        maps = {}
        for kind in ('Color', 'NormalGL', 'ORM', 'AO'):
            targets = []
            for region, mat in zip(('Character', 'Suit'), obj.data.materials):
                name = 'T_Player_%s_%s' % (region, kind)
                image = bpy.data.images.get(name) or bpy.data.images.new(
                    name, width=2048, height=2048, alpha=False)
                image.colorspace_settings.name = 'sRGB' if kind == 'Color' else 'Non-Color'
                nodes = mat.node_tree.nodes
                target = nodes.get('BAKE_TARGET') or nodes.new('ShaderNodeTexImage')
                target.name = 'BAKE_TARGET'
                target.image = image
                nodes.active = target
                targets.append(image)
                maps[(region, kind)] = image
            _activate(obj)
            src.hide_viewport = False
            src.hide_render = False
            src.hide_set(False)
            src.select_set(True)
            if kind == 'ORM':
                nt.links.new(orm.outputs['Color'], emission.inputs['Color'])
                nt.links.new(emission.outputs[0], output.inputs['Surface'])
            else:
                nt.links.new(original_surface, output.inputs['Surface'])
            kwargs = dict(type={'Color': 'DIFFUSE', 'NormalGL': 'NORMAL',
                                'ORM': 'EMIT', 'AO': 'AO'}[kind],
                          use_selected_to_active=True,
                          cage_extrusion=CAGE_EXTRUSION,
                          max_ray_distance=MAX_RAY_DISTANCE,
                          margin=MARGIN, margin_type='ADJACENT_FACES',
                          use_clear=True)
            if kind == 'Color':
                kwargs['pass_filter'] = {'COLOR'}
            if kind == 'NormalGL':
                kwargs.update(normal_space='TANGENT', normal_g='POS_Y')
            assert 'FINISHED' in bpy.ops.object.bake(**kwargs)
            report['completed'].append(kind)
            REPORT.write_text(json.dumps(report, indent=2))

        report['inpaint'] = {}
        for region in ('Character', 'Suit'):
            images = {kind: maps[(region, kind)] for kind in ('Color', 'NormalGL', 'ORM', 'AO')}
            report['inpaint'][region] = inpaint(images)
        for region in ('Character', 'Suit'):
            for kind in ('Color', 'NormalGL', 'ORM', 'AO'):
                image = maps[(region, kind)]
                image.filepath_raw = str(ASSET / 'textures' / ('%s.png' % image.name))
                image.file_format = 'PNG'
                image.save()

        for region, mat in zip(('Character', 'Suit'), obj.data.materials):
            nodes = mat.node_tree.nodes
            links = mat.node_tree.links
            nodes.clear()
            out = nodes.new('ShaderNodeOutputMaterial')
            bsdf = nodes.new('ShaderNodeBsdfPrincipled')
            links.new(bsdf.outputs[0], out.inputs['Surface'])
            images = {}
            for kind in ('Color', 'NormalGL', 'ORM'):
                n = nodes.new('ShaderNodeTexImage')
                n.image = maps[(region, kind)]
                n.name = kind
                images[kind] = n
            links.new(images['Color'].outputs['Color'], bsdf.inputs['Base Color'])
            norm = nodes.new('ShaderNodeNormalMap')
            links.new(images['NormalGL'].outputs['Color'], norm.inputs['Color'])
            links.new(norm.outputs['Normal'], bsdf.inputs['Normal'])
            split = nodes.new('ShaderNodeSeparateColor')
            links.new(images['ORM'].outputs['Color'], split.inputs[0])
            links.new(split.outputs[1], bsdf.inputs['Roughness'])
            links.new(split.outputs[2], bsdf.inputs['Metallic'])
        report['status'] = 'REBAKED_AWAITING_VISUAL_QA'
    except Exception:
        report['status'] = 'FAILED'
        report['error'] = traceback.format_exc()
    finally:
        src.data.materials.clear()
        for mat in old_mats:
            src.data.materials.append(mat)
        if temporary is not None:
            bpy.data.materials.remove(temporary)
        src.hide_viewport, src.hide_render = visibility[:2]
        src.hide_set(visibility[2])
        scene.render.engine = engine
        mod.show_viewport = True
        mod.show_render = True
        report['elapsed_s'] = round(time.time() - start, 1)
        REPORT.write_text(json.dumps(report, indent=2) + '\n')
    return None


NORMAL_Z_FLOOR = 0.40


def clamp_normals(z_floor=NORMAL_Z_FLOOR):
    """Acota la componente Z de los normal maps tangenciales y renormaliza.

    El bake proyecta una fuente de 233k con mechones y grietas profundas sobre
    una superficie de retopo suave. Donde la desviacion es extrema el normal
    map guarda vectores con Z <= 0, es decir apuntando HACIA DENTRO de la
    superficie: fisicamente invalido en espacio tangente. EEVEE y Unreal los
    sombrean en negro, que es el origen de los parches oscuros del pelo.

    Se recorta Z al suelo indicado y se reescala XY para conservar el vector
    unitario, de modo que solo cambian los texels degenerados.
    """
    report = {}
    for region in ('Character', 'Suit'):
        image = bpy.data.images.get('T_Player_%s_NormalGL' % region)
        if image is None:
            report[region] = 'MISSING'
            continue
        data = _read(image)
        n = data[:, :, :3] * 2.0 - 1.0
        z = n[:, :, 2]
        bad = z < z_floor
        count = int(bad.sum())
        z_new = np.where(bad, z_floor, z)
        xy = n[:, :, :2]
        xy_len = np.sqrt((xy ** 2).sum(axis=2))
        target = np.sqrt(np.maximum(0.0, 1.0 - z_new ** 2))
        scale = np.where(xy_len > 1e-6, target / np.maximum(xy_len, 1e-6), 0.0)
        scale = np.where(bad, scale, 1.0)
        n[:, :, 0] *= scale
        n[:, :, 1] *= scale
        n[:, :, 2] = z_new
        data[:, :, :3] = (n + 1.0) * 0.5
        _write(image, data)
        image.filepath_raw = str(ASSET / 'textures' / ('%s.png' % image.name))
        image.file_format = 'PNG'
        image.save()
        report[region] = {'texels_clamped': count,
                          'pct': round(100.0 * count / z.size, 2)}
    path = ASSET / 'docs/normal_clamp.json'
    path.write_text(json.dumps({'z_floor': z_floor, 'regions': report}, indent=2) + '\n')
    return report
