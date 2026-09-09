"""Morph targets faciales del protagonista.

Decision registrada 2026-09-08: la cabeza tiene 249 vertices de cara con arista
media de 8,9 mm y ~18 vertices por ojo. A esa densidad son viables las
deformaciones de gran escala (mandibula, cejas) y NO lo es un parpadeo, que
necesita bucles de parpado inexistentes. Se autorizan tres morphs y el parpadeo
queda como limitacion conocida hasta una eventual retopologia densa de cabeza.

El rig es un esqueleto UE5 estandar sin huesos faciales (solo `neck_01` y
`head`), asi que morph target es la unica via.

Anclajes obtenidos del render QA con camara ortografica, que da una conversion
exacta pixel -> z (0.34 m sobre 1100 px, centro en z = 1.715). La escala se
valido contra la coronilla, que cae en z = 1.8287 frente al z = 1.83 real.
Las lineas oscuras de la columna central de la cara resultaron:

    linea de ojos y ceja   z = 1.6966
    base de la nariz       z = 1.6430
    linea de la boca       z = 1.6195
    cuello y cuello duro   z = 1.5688
    nacimiento del pelo    z ~ 1.7650
    coronilla              z   1.8300

La silueta de la malla por si sola induce a error: la franja estrecha de
z 1.58-1.63 no es el cuello sino el menton y la mandibula, y el ensanchamiento
de z 1.69-1.75 es craneo MAS pelo.
"""
import json
import math
from pathlib import Path
import bpy
from mathutils import Matrix, Vector

ROOT = Path(r'C:/Users/MAXIMO/Desktop/Astraeon')
ASSET = ROOT / 'graphics/characters/main_player'
MESH = 'SK_Astraeon_Player'

CHIN_Z = 1.595          # cuerpo del menton
MOUTH_Z = 1.632         # justo sobre la linea de labios: el labio superior no baja
BROW_LO, BROW_HI = 1.692, 1.722
HINGE = Vector((0.0, 0.035, 1.690))     # articulacion mandibular, delante de la oreja
JAW_DEGREES = 6.0
BROW_RAISE_M = 0.007
BROW_FURROW_M = 0.005


def _smoothstep(edge0, edge1, x):
    if edge0 == edge1:
        return 0.0
    t = max(0.0, min(1.0, (x - edge0) / (edge1 - edge0)))
    return t * t * (3 - 2 * t)


def _front(y, near=-0.04, far=0.0):
    """1 en la cara, 0 en la nuca; evita arrastrar el craneo."""
    return _smoothstep(far, near, y)


def _ensure_basis(obj):
    if obj.data.shape_keys is None:
        obj.shape_key_add(name='Basis', from_mix=False)
    return obj.data.shape_keys


def _add(obj, name, displace):
    keys = _ensure_basis(obj)
    existing = keys.key_blocks.get(name)
    if existing is not None:
        obj.shape_key_remove(existing)
    key = obj.shape_key_add(name=name, from_mix=False)
    key.slider_min, key.slider_max = 0.0, 1.0
    basis = keys.key_blocks['Basis'].data
    moved = 0
    largest = 0.0
    for index, point in enumerate(key.data):
        offset = displace(basis[index].co)
        if offset.length > 1e-6:
            point.co = basis[index].co + offset
            moved += 1
            largest = max(largest, offset.length)
    return {'name': name, 'moved_vertices': moved,
            'max_offset_mm': round(largest * 1000, 2)}


JAW_FLOOR_Z = 1.562      # por debajo empieza el cuello: no debe moverse
MOUTH_CORNER = Vector((0.0, -0.090, 1.6195))   # comisura, sobre la linea de labios


def _mandible(co):
    """Distancia con signo al plano de la mandibula, en el corte y-z.

    Una banda en Z no sirve: la linea mandibular sube desde el menton hasta la
    bisagra, asi que un corte horizontal deja fuera la rama de la mandibula y el
    morph estira el menton en lugar de rotar la pieza entera. El limite real es
    el plano que pasa por la bisagra y la comisura de los labios; por debajo
    esta la mandibula, por encima el craneo.
    """
    direction = Vector((MOUTH_CORNER.y - HINGE.y, MOUTH_CORNER.z - HINGE.z))
    direction.normalize()
    normal = Vector((direction.y, -direction.x))     # positivo hacia el craneo
    return (co.y - HINGE.y) * normal.x + (co.z - HINGE.z) * normal.y


def jaw_open(co):
    weight = (_smoothstep(0.012, -0.008, _mandible(co))
              * _smoothstep(JAW_FLOOR_Z, JAW_FLOOR_Z + 0.022, co.z)
              * _front(co.y, near=-0.02, far=0.055))
    if weight <= 0.0:
        return Vector((0, 0, 0))
    rotation = Matrix.Rotation(math.radians(JAW_DEGREES) * weight, 3, 'X')
    return (rotation @ (co - HINGE)) + HINGE - co


def brow_raise(co):
    band = _smoothstep(BROW_LO, (BROW_LO + BROW_HI) / 2, co.z) * \
           _smoothstep(BROW_HI, (BROW_LO + BROW_HI) / 2, co.z)
    weight = band * _front(co.y, near=-0.05)
    return Vector((0, 0, BROW_RAISE_M * weight))


def brow_furrow(co):
    band = _smoothstep(BROW_LO, (BROW_LO + BROW_HI) / 2, co.z) * \
           _smoothstep(BROW_HI, (BROW_LO + BROW_HI) / 2, co.z)
    weight = band * _front(co.y, near=-0.05)
    inward = -math.copysign(1.0, co.x) if abs(co.x) > 1e-5 else 0.0
    return Vector((inward * BROW_FURROW_M * weight * 0.8,
                   0.0,
                   -BROW_FURROW_M * weight * 0.5))


MORPHS = (('jaw_open', jaw_open),
          ('brow_raise', brow_raise),
          ('brow_furrow', brow_furrow))


def build():
    obj = bpy.data.objects[MESH]
    report = {'status': 'MORPHS_BUILT', 'mesh': MESH, 'morphs': [],
              'blink': 'no viable con esta topologia (~18 vertices por ojo); '
                       'limitacion conocida hasta retopologia densa de cabeza'}
    for name, function in MORPHS:
        report['morphs'].append(_add(obj, name, function))
    keys = obj.data.shape_keys
    for block in keys.key_blocks:
        block.value = 0.0
    report['key_blocks'] = [b.name for b in keys.key_blocks]
    (ASSET / 'docs/facial_morphs.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def preview(name, value=1.0):
    obj = bpy.data.objects[MESH]
    for block in obj.data.shape_keys.key_blocks:
        block.value = value if block.name == name else 0.0
    return {'active': name, 'value': value}
