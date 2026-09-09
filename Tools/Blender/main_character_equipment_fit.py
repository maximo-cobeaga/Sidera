"""Asienta la mochila sobre la espalda real del traje.

Auditoria 2026-09-08 (QA_Rebake_Body_Side.png): la mochila se construyo con un
frente PLANO, con el centro de sus anillos casi constante en y = 0.205..0.218.
La espalda del traje, en cambio, describe una curva: y = 0.074 a la altura de
la cintura y y = 0.191 en los omoplatos. El resultado medido era

    z = 1.09  ->  87 mm de hueco   (la mochila flota)
    z = 1.40  ->  51 mm de invasion (la mochila entra en el torso)

Aqui no se reescriben las cotas del generador una por una: se mide la espalda
del cuerpo y se aplica un desplazamiento en Y funcion de la altura, de modo que
los paneles de detalle acompanan el mismo campo y la pieza conserva su volumen.
"""
import json
from pathlib import Path
import bpy
import numpy as np

ROOT = Path(r'C:/Users/MAXIMO/Desktop/Astraeon')
ASSET = ROOT / 'graphics/characters/main_player'
BODY = 'SK_Astraeon_Player'
PACK = 'SK_Astraeon_Backpack'

CLEARANCE = 0.002   # 2 mm: contacto visible sin z-fighting con el traje
HALF_WIDTH = 0.16   # franja central del torso que soporta la pieza
SLAB = 0.012        # medio espesor de la franja de muestreo en Z


def _smooth(values, passes=2):
    out = np.array(values, dtype=float)
    for _ in range(passes):
        padded = np.concatenate(([out[0]], out, [out[-1]]))
        out = (padded[:-2] + 2 * padded[1:-1] + padded[2:]) / 4
    return out


def fit_backpack(clearance=CLEARANCE):
    body = bpy.data.objects[BODY]
    pack = bpy.data.objects[PACK]
    body_co = np.array([v.co[:] for v in body.data.vertices])
    pack_co = np.array([v.co[:] for v in pack.data.vertices])

    z_lo, z_hi = float(pack_co[:, 2].min()), float(pack_co[:, 2].max())
    samples = np.linspace(z_lo, z_hi, 24)

    torso, front = [], []
    for z in samples:
        band = (np.abs(body_co[:, 2] - z) < SLAB) & (np.abs(body_co[:, 0]) < HALF_WIDTH)
        torso.append(float(body_co[band, 1].max()) if band.sum() > 3 else np.nan)
        pband = np.abs(pack_co[:, 2] - z) < SLAB
        front.append(float(pack_co[pband, 1].min()) if pband.sum() > 3 else np.nan)

    torso = np.array(torso)
    front = np.array(front)
    valid = ~(np.isnan(torso) | np.isnan(front))
    torso = np.interp(samples, samples[valid], torso[valid])
    front = np.interp(samples, samples[valid], front[valid])
    offset = _smooth(torso + clearance - front)

    before = {'gap_min_mm': round(float((front - torso).min()) * 1000, 1),
              'gap_max_mm': round(float((front - torso).max()) * 1000, 1)}

    for vert in pack.data.vertices:
        vert.co.y += float(np.interp(vert.co.z, samples, offset))
    pack.data.update()

    after_co = np.array([v.co[:] for v in pack.data.vertices])
    residual = []
    for i, z in enumerate(samples):
        pband = np.abs(after_co[:, 2] - z) < SLAB
        if pband.sum() > 3:
            residual.append(float(after_co[pband, 1].min()) - torso[i])
    residual = np.array(residual)

    report = {
        'status': 'BACKPACK_FITTED',
        'clearance_m': clearance,
        'offset_range_mm': [round(float(offset.min()) * 1000, 1),
                            round(float(offset.max()) * 1000, 1)],
        'before': before,
        'after': {'gap_min_mm': round(float(residual.min()) * 1000, 1),
                  'gap_max_mm': round(float(residual.max()) * 1000, 1)},
        'dimensions': [round(v, 4) for v in pack.dimensions],
    }
    (ASSET / 'docs/equipment_fit.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def rebuild_backpack(detail=True):
    """Regenera la mochila desde los parametros corregidos del generador.

    No se toca la malla a mano: se aparta la pieza anterior y se reconstruye por
    el mismo camino de codigo que la creo, para que el archivo y el generador
    sigan diciendo lo mismo.
    """
    import importlib
    import main_character_equipment as eq
    importlib.reload(eq)

    old = bpy.data.objects.get(PACK)
    archived = None
    if old is not None:
        archived = 'ARCHIVE_Backpack_PreFit'
        if bpy.data.objects.get(archived) is None:
            old.name = archived
            old.data.name = archived
            old.hide_render = True
            old.hide_set(True)
        else:
            bpy.data.objects.remove(old, do_unlink=True)
            archived += ' (ya existia; se descarto la copia intermedia)'

    obj = eq.backpack(detail)

    report = measure_contact(obj)
    report['status'] = 'BACKPACK_REBUILT'
    report['archived'] = archived
    report['vertices'] = len(obj.data.vertices)
    report['triangles'] = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    report['dimensions'] = [round(v, 4) for v in obj.dimensions]
    (ASSET / 'docs/equipment_fit.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def measure_contact(obj=None):
    """Distancia real pieza <-> cuerpo con BVH, no comparacion de cotas.

    Para cada vertice de la pieza se busca el punto mas cercano del cuerpo y se
    firma la distancia contra la normal de esa cara: negativo = dentro del
    torso. Es la unica medida que distingue apoyo de invasion.
    """
    from mathutils.bvhtree import BVHTree
    obj = obj or bpy.data.objects[PACK]
    body = bpy.data.objects[BODY]
    me = body.data
    me.calc_loop_triangles()
    tree = BVHTree.FromPolygons([v.co.copy() for v in me.vertices],
                                [tuple(t.vertices) for t in me.loop_triangles],
                                all_triangles=True)
    inside, distances = 0, []
    deepest = 0.0
    for vert in obj.data.vertices:
        hit, normal, index, distance = tree.find_nearest(vert.co)
        if hit is None:
            continue
        signed = (vert.co - hit).dot(normal)
        distances.append(distance)
        if signed < 0:
            inside += 1
            deepest = max(deepest, distance)
    distances.sort()
    return {
        'vertices_inside_body': inside,
        'deepest_penetration_mm': round(deepest * 1000, 1),
        'closest_mm': round(distances[0] * 1000, 1),
        'median_mm': round(distances[len(distances) // 2] * 1000, 1),
        'farthest_mm': round(distances[-1] * 1000, 1),
    }
