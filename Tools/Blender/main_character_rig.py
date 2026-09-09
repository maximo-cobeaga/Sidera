"""Construccion del rig del personaje principal de ASTRAEON.

Todas las posiciones provienen de medicion directa sobre la malla generada
(secciones por Z/X, centroides de seccion y curvas mediales de los dedos por
clustering radial). No hay posiciones inventadas ni rigging remoto.

Nomenclatura compatible con el esqueleto humano estandar de Unreal Engine.
Convencion del proyecto: el personaje mira a -Y, +X es su lado izquierdo (_l).
Al exportar FBX con Forward -Y / Up Z, Blender +X -> UE -Y = lado izquierdo.
"""
import bpy
from mathutils import Vector

RIG_NAME = 'SKEL_Astraeon_Player'
RIG_COLLECTION = 'ASTRAEON_RIG'

# ---------------------------------------------------------------- landmarks
SPINE = [
    ('pelvis',   (0.0,  0.000, 0.980), (0.0,  0.000, 1.060)),
    ('spine_01', (0.0,  0.000, 1.060), (0.0,  0.005, 1.160)),
    ('spine_02', (0.0,  0.005, 1.160), (0.0,  0.015, 1.280)),
    ('spine_03', (0.0,  0.015, 1.280), (0.0,  0.020, 1.520)),
    ('neck_01',  (0.0,  0.020, 1.520), (0.0,  0.005, 1.640)),
    ('head',     (0.0,  0.005, 1.640), (0.0, -0.005, 1.800)),
]

# cadenas laterales definidas con X positivo (lado _l); _r se refleja en X
ARM = [
    ('clavicle', 'spine_03', (0.020, 0.010, 1.480), (0.175, 0.035, 1.437)),
    # el codo se situa al 54% de hombro->muneca; el minimo de seccion medido en
    # x=0.42 corresponde a la placa del antebrazo, no a la articulacion.
    ('upperarm', 'clavicle', (0.175, 0.035, 1.437), (0.465, 0.071, 1.364)),
    ('lowerarm', 'upperarm', (0.465, 0.071, 1.364), (0.697, -0.020, 1.345)),
    ('hand',     'lowerarm', (0.697, -0.020, 1.345), None),   # tail = MCP medio
]
LEG = [
    ('thigh', 'pelvis', (0.098, -0.020, 0.945), (0.160, -0.005, 0.515)),
    ('calf',  'thigh',  (0.160, -0.005, 0.515), (0.200,  0.010, 0.115)),
    ('foot',  'calf',   (0.200,  0.010, 0.115), (0.213, -0.096, 0.030)),
    ('ball',  'foot',   (0.213, -0.096, 0.030), (0.215, -0.175, 0.025)),
]

# curvas mediales medidas (MCP -> punta), lado _l; se remuestrean por longitud
# de arco para repartir falange proximal / media / distal.
DIGITS = {
    'index': [(0.7729, -0.0710, 1.3596), (0.7890, -0.0699, 1.3619),
              (0.8074, -0.0693, 1.3601), (0.8187, -0.0788, 1.3541),
              (0.8324, -0.0811, 1.3529), (0.8465, -0.0871, 1.3509),
              (0.8572, -0.0915, 1.3518), (0.8717, -0.0995, 1.3506),
              (0.8824, -0.1035, 1.3498), (0.8969, -0.1091, 1.3472),
              (0.9057, -0.1102, 1.3492)],
    'middle': [(0.7912, -0.0441, 1.3371), (0.8059, -0.0447, 1.3368),
               (0.8202, -0.0505, 1.3328), (0.8308, -0.0571, 1.3285),
               (0.8447, -0.0651, 1.3262), (0.8588, -0.0701, 1.3259),
               (0.8704, -0.0749, 1.3258), (0.8844, -0.0810, 1.3193),
               (0.8956, -0.0864, 1.3171), (0.9093, -0.0924, 1.3112),
               (0.9183, -0.0944, 1.3115)],
    'ring': [(0.7859, -0.0257, 1.3297), (0.7982, -0.0275, 1.3259),
             (0.8122, -0.0309, 1.3216), (0.8205, -0.0420, 1.3130),
             (0.8327, -0.0470, 1.3074), (0.8452, -0.0505, 1.3040),
             (0.8559, -0.0537, 1.3003), (0.8687, -0.0608, 1.2940),
             (0.8791, -0.0651, 1.2927), (0.8924, -0.0700, 1.2860),
             (0.9006, -0.0711, 1.2870)],
    'pinky': [(0.7771, -0.0198, 1.3004), (0.7898, -0.0209, 1.2980),
              (0.8025, -0.0232, 1.2956), (0.8120, -0.0301, 1.2912),
              (0.8231, -0.0348, 1.2874), (0.8323, -0.0384, 1.2802),
              (0.8431, -0.0428, 1.2738), (0.8513, -0.0457, 1.2694),
              (0.8632, -0.0485, 1.2624), (0.8695, -0.0507, 1.2602)],
    'thumb': [(0.7088, -0.0669, 1.3361), (0.7177, -0.0805, 1.3315),
              (0.7243, -0.0903, 1.3337), (0.7350, -0.0957, 1.3306),
              (0.7406, -0.1014, 1.3272), (0.7419, -0.1129, 1.3169),
              (0.7449, -0.1206, 1.3109), (0.7483, -0.1276, 1.3043),
              (0.7532, -0.1348, 1.2985), (0.7569, -0.1436, 1.2939),
              (0.7616, -0.1595, 1.2897)],
}
DIGIT_ORDER = ['thumb', 'index', 'middle', 'ring', 'pinky']
DIGIT_SPLITS = {'thumb': (0.0, 0.42, 0.72, 1.0)}
DEFAULT_SPLIT = (0.0, 0.45, 0.72, 1.0)

# Longitud util de cada dedo desde el nudillo (MCP) hasta la punta, en metros.
# El MCP se obtiene retrocediendo esta distancia sobre la curva medial medida:
# el radio de seccion decae de forma continua y no marca el nudillo por si solo.
FINGER_LEN = {'index': 0.086, 'middle': 0.095, 'ring': 0.089, 'pinky': 0.070}


def trim_to_length(points, length):
    """Recorta la polilinea dejando solo los `length` metros finales."""
    pts = [Vector(p) for p in points]
    total = 0.0
    for i in range(len(pts) - 1, 0, -1):
        seg = (pts[i] - pts[i - 1]).length
        if total + seg >= length:
            t = (length - total) / seg if seg > 1e-9 else 0.0
            return [pts[i].lerp(pts[i - 1], t)] + pts[i:]
        total += seg
    return pts


def resample(points, fractions):
    """Puntos sobre la polilinea a las fracciones dadas de longitud de arco."""
    pts = [Vector(p) for p in points]
    seg = [(pts[i + 1] - pts[i]).length for i in range(len(pts) - 1)]
    total = sum(seg)
    cum = [0.0]
    for s in seg:
        cum.append(cum[-1] + s)
    out = []
    for f in fractions:
        target = f * total
        if target >= total:
            out.append(pts[-1].copy())
            continue
        for i in range(len(seg)):
            if cum[i + 1] >= target:
                t = (target - cum[i]) / seg[i] if seg[i] > 1e-9 else 0.0
                out.append(pts[i].lerp(pts[i + 1], t))
                break
    return out


def palm_normal(side_sign):
    """Normal del plano de la palma, medida sobre los MCP y el eje del dedo medio."""
    across = Vector(DIGITS['pinky'][0]) - Vector(DIGITS['index'][0])
    along = Vector(DIGITS['middle'][-1]) - Vector(DIGITS['middle'][0])
    n = across.cross(along).normalized()
    return (n.x * side_sign, n.y, n.z)


def build_rig(context=None):
    ctx = context or bpy.context
    if bpy.data.objects.get(RIG_NAME):
        raise RuntimeError('El rig ya existe: inspeccionarlo antes de reconstruir.')

    arm_data = bpy.data.armatures.new(RIG_NAME + '_Data')
    rig = bpy.data.objects.new(RIG_NAME, arm_data)
    bpy.data.collections[RIG_COLLECTION].objects.link(rig)

    if ctx.object and ctx.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True)
    ctx.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode='EDIT')
    eb = arm_data.edit_bones
    created = []

    def bone(name, head, tail, parent=None, deform=True,
             roll_to=(0.0, -1.0, 0.0), connect=False):
        h, t = Vector(head), Vector(tail)
        if (t - h).length < 1e-3:
            raise ValueError('Hueso de longitud nula: ' + name)
        b = eb.new(name)
        b.head, b.tail = h, t
        b.align_roll(Vector(roll_to))
        b.use_deform = deform
        if parent:
            b.parent = eb[parent]
            if connect and (eb[parent].tail - h).length < 1e-4:
                b.use_connect = True
        created.append(name)
        return b

    bone('root', (0, 0, 0), (0, -0.150, 0), deform=False)
    parent = 'root'
    for name, head, tail in SPINE:
        bone(name, head, tail, parent, connect=(parent != 'root'))
        parent = name

    roll_limb = (0.0, 0.0, 1.0)
    roll_leg = (0.0, -1.0, 0.0)
    for side, sgn in (('l', 1.0), ('r', -1.0)):
        def mir(p):
            return (p[0] * sgn, p[1], p[2])

        curves = {d: (trim_to_length(DIGITS[d], FINGER_LEN[d])
                      if d in FINGER_LEN else [Vector(p) for p in DIGITS[d]])
                  for d in DIGIT_ORDER}
        mcps = [curves[d][0] for d in FINGER_LEN]
        hand_tail = tuple(sum(p[i] for p in mcps) / len(mcps) for i in range(3))

        for stem, par, head, tail in ARM:
            par_name = par if par == 'spine_03' else par + '_' + side
            bone(stem + '_' + side, mir(head), mir(tail or hand_tail), par_name,
                 roll_to=roll_limb, connect=(stem != 'clavicle'))

        pn = palm_normal(sgn)
        for digit in DIGIT_ORDER:
            nodes = resample(curves[digit], DIGIT_SPLITS.get(digit, DEFAULT_SPLIT))
            par_name = 'hand_' + side
            for i in range(3):
                nm = '%s_%02d_%s' % (digit, i + 1, side)
                bone(nm, mir(nodes[i]), mir(nodes[i + 1]), par_name,
                     roll_to=pn, connect=(i > 0))
                par_name = nm

        for stem, par, head, tail in LEG:
            par_name = par if par == 'pelvis' else par + '_' + side
            bone(stem + '_' + side, mir(head), mir(tail), par_name,
                 roll_to=roll_leg, connect=(stem != 'thigh'))

    # cadenas IK con la jerarquia estandar de Unreal (no deforman)
    bone('ik_foot_root', (0, 0, 0), (0, -0.100, 0), 'root', deform=False)
    bone('ik_hand_root', (0, 0, 0), (0, -0.100, 0), 'root', deform=False)
    foot_head = LEG[2][2]
    hand_head = ARM[3][2]
    for side, sgn in (('l', 1.0), ('r', -1.0)):
        bone('ik_foot_' + side,
             (foot_head[0] * sgn, foot_head[1], foot_head[2]),
             (foot_head[0] * sgn, foot_head[1] - 0.100, foot_head[2]),
             'ik_foot_root', deform=False)
    bone('ik_hand_gun',
         (hand_head[0] * -1.0, hand_head[1], hand_head[2]),
         (hand_head[0] * -1.0, hand_head[1] - 0.100, hand_head[2]),
         'ik_hand_root', deform=False)
    for side, sgn in (('l', 1.0), ('r', -1.0)):
        bone('ik_hand_' + side,
             (hand_head[0] * sgn, hand_head[1], hand_head[2]),
             (hand_head[0] * sgn, hand_head[1] - 0.100, hand_head[2]),
             'ik_hand_gun', deform=False)

    # sockets de equipamiento (no deforman); alias socket_* usados por el runtime
    for side, sgn in (('l', 1.0), ('r', -1.0)):
        hb = eb['hand_' + side]
        centre = hb.head.lerp(hb.tail, 0.55)      # centro de la palma medido
        axis = (hb.tail - hb.head).normalized()
        grip = tuple(centre)
        fwd = tuple(centre + axis * 0.060)
        for nm in ('weapon_' + side, 'tool_' + side, 'socket_tool_' + side):
            bone(nm, grip, fwd, 'hand_' + side, deform=False)
        bone('hip_%s_attach' % side, (0.150 * sgn, 0.020, 1.020),
             (0.210 * sgn, 0.020, 1.020), 'pelvis', deform=False)
    bone('back_attach', (0.0, 0.120, 1.400), (0.0, 0.200, 1.400),
         'spine_03', deform=False)
    bone('socket_backpack', (0.0, 0.120, 1.400), (0.0, 0.200, 1.400),
         'spine_03', deform=False)
    bone('socket_helmet', (0.0, 0.005, 1.640), (0.0, 0.005, 1.760),
         'head', deform=False)

    bpy.ops.object.mode_set(mode='OBJECT')
    return rig, created


if __name__ == '__main__':
    build_rig()
