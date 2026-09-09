"""Autoria de las Actions del personaje principal de ASTRAEON.

Convenciones de eje medidas sobre SKEL_Astraeon_Player (no asumidas):

    upperarm  +X  brazo arriba          -X  brazo abajo
    upperarm  +Z  izq atras / der adel  (ejes locales espejados)
    lowerarm  +X  flexion de codo
    thigh     +X  flexion de cadera (pierna adelante)
    calf      -X  flexion de rodilla
    foot      +X  punta arriba
    dedos     +X  cierre hacia la palma
    spine     +X  inclinacion adelante  +Z  torsion (cabeza hacia -X)
    neck/head +X  inclinacion adelante  +Z  giro a la derecha del personaje

Por eso `mirror()` intercambia _l/_r conservando X y negando Y/Z: es lo que
producen los ejes locales reales del rig, no una convencion elegida a mano.

Todas las Actions son in-place (sin root motion). El desplazamiento real lo
aporta el Animation Blueprint de Unreal; `travel_speed_ms` queda documentado
en animation_contract.json para calibrar la velocidad de reproduccion.
"""
import bpy
from mathutils import Vector

RIG_NAME = 'SKEL_Astraeon_Player'
FPS = 30
FINGERS = ('index', 'middle', 'ring', 'pinky')


# --------------------------------------------------------------- utilidades
def merge(*poses):
    out = {}
    for p in poses:
        out.update(p)
    return out


def mirror(pose):
    """Refleja una pose: intercambia lado y niega los ejes de eje local espejado."""
    out = {}
    for bone, val in pose.items():
        if bone.endswith('_l'):
            name = bone[:-2] + '_r'
        elif bone.endswith('_r'):
            name = bone[:-2] + '_l'
        else:
            name = bone
        if len(val) == 4 and val[0] == 'loc':
            out[name] = ('loc', -val[1], val[2], val[3])
        else:
            out[name] = (val[0], -val[1], -val[2])
    return out


def hand(side, curl, thumb=None, index=None, spread=0.0):
    """Pose de una mano. `curl` cierra los cuatro dedos; `index` lo sobreescribe."""
    p = {}
    for i, f in enumerate(FINGERS):
        c = index if (f == 'index' and index is not None) else curl
        sp = spread * (i - 1.5) * (1.0 if side == 'l' else -1.0)
        p['%s_01_%s' % (f, side)] = (c * 0.95, 0.0, sp)
        p['%s_02_%s' % (f, side)] = (c * 1.10, 0.0, 0.0)
        p['%s_03_%s' % (f, side)] = (c * 0.85, 0.0, 0.0)
    t = curl * 0.55 if thumb is None else thumb
    p['thumb_01_%s' % side] = (t * 0.70, 0.0, 0.0)
    p['thumb_02_%s' % side] = (t * 0.95, 0.0, 0.0)
    p['thumb_03_%s' % side] = (t * 0.80, 0.0, 0.0)
    return p


def hands(curl, thumb=None, index=None, spread=0.0):
    return merge(hand('l', curl, thumb, index, spread),
                 hand('r', curl, thumb, index, spread))


def pelvis(up=0.0, side=0.0, back=0.0):
    """Traslacion de pelvis en espacio de hueso: local Y = +Z mundo, local Z = -Y mundo."""
    return {'pelvis': ('loc', side, up, back)}


# ------------------------------------------------------------- poses tronco
RELAXED_HANDS = hands(0.26, thumb=0.30, spread=0.05)

STAND = merge(
    {'upperarm_l': (-1.245, 0.0, -0.130), 'upperarm_r': (-1.245, 0.0, 0.130),
     'lowerarm_l': (0.300, 0.0, 0.0), 'lowerarm_r': (0.300, 0.0, 0.0),
     'clavicle_l': (0.0, 0.0, 0.0), 'clavicle_r': (0.0, 0.0, 0.0),
     'thigh_l': (0.02, 0.0, -0.055), 'thigh_r': (0.02, 0.0, 0.055),
     'calf_l': (-0.06, 0.0, 0.0), 'calf_r': (-0.06, 0.0, 0.0),
     'foot_l': (0.03, 0.0, 0.0), 'foot_r': (0.03, 0.0, 0.0),
     'spine_01': (0.015, 0.0, 0.0), 'spine_02': (0.02, 0.0, 0.0),
     'spine_03': (0.01, 0.0, 0.0), 'neck_01': (0.02, 0.0, 0.0),
     'head': (-0.01, 0.0, 0.0)},
    pelvis(),          # presente siempre: si falta en una clave, la curva de
                       # traslacion arrancaria en el valor de la clave siguiente
    RELAXED_HANDS)

# --------------------------------------------------------------- gaits
# Medio ciclo con la pierna derecha al frente; la otra mitad es su espejo.
WALK_HALF = [
    # phi = 0.000  CONTACTO derecho
    merge(STAND, pelvis(up=-0.012),
          {'thigh_r': (0.42, 0.0, 0.05), 'calf_r': (-0.14, 0.0, 0.0), 'foot_r': (0.20, 0.0, 0.0),
           'thigh_l': (-0.34, 0.0, -0.05), 'calf_l': (-0.22, 0.0, 0.0), 'foot_l': (-0.28, 0.0, 0.0),
           'upperarm_l': (-1.245, 0.0, -0.45), 'upperarm_r': (-1.245, 0.0, -0.19),
           'lowerarm_l': (0.46, 0.0, 0.0), 'lowerarm_r': (0.24, 0.0, 0.0),
           'spine_02': (0.05, 0.0, 0.07)}),
    # phi = 0.125  APOYO / punto bajo
    merge(STAND, pelvis(up=-0.040, side=0.012),
          {'thigh_r': (0.24, 0.0, 0.05), 'calf_r': (-0.32, 0.0, 0.0), 'foot_r': (0.02, 0.0, 0.0),
           'thigh_l': (-0.14, 0.0, -0.05), 'calf_l': (-0.62, 0.0, 0.0), 'foot_l': (-0.10, 0.0, 0.0),
           'upperarm_l': (-1.245, 0.0, -0.30), 'upperarm_r': (-1.245, 0.0, -0.12),
           'lowerarm_l': (0.40, 0.0, 0.0), 'lowerarm_r': (0.26, 0.0, 0.0),
           'spine_02': (0.04, 0.0, 0.04)}),
    # phi = 0.250  PASO / punto alto
    merge(STAND, pelvis(up=0.008, side=0.016),
          {'thigh_r': (0.02, 0.0, 0.05), 'calf_r': (-0.10, 0.0, 0.0), 'foot_r': (0.00, 0.0, 0.0),
           'thigh_l': (0.12, 0.0, -0.05), 'calf_l': (-0.88, 0.0, 0.0), 'foot_l': (0.18, 0.0, 0.0),
           'upperarm_l': (-1.245, 0.0, -0.13), 'upperarm_r': (-1.245, 0.0, 0.13),
           'lowerarm_l': (0.32, 0.0, 0.0), 'lowerarm_r': (0.32, 0.0, 0.0),
           'spine_02': (0.02, 0.0, 0.0)}),
    # phi = 0.375  IMPULSO
    merge(STAND, pelvis(up=-0.004, side=0.010),
          {'thigh_r': (-0.20, 0.0, 0.05), 'calf_r': (-0.08, 0.0, 0.0), 'foot_r': (-0.24, 0.0, 0.0),
           'thigh_l': (0.34, 0.0, -0.05), 'calf_l': (-0.52, 0.0, 0.0), 'foot_l': (0.14, 0.0, 0.0),
           'upperarm_l': (-1.245, 0.0, 0.06), 'upperarm_r': (-1.245, 0.0, 0.30),
           'lowerarm_l': (0.28, 0.0, 0.0), 'lowerarm_r': (0.40, 0.0, 0.0),
           'spine_02': (0.03, 0.0, -0.03)}),
]

RUN_HALF = [
    merge(STAND, pelvis(up=-0.030),
          {'thigh_r': (0.72, 0.0, 0.04), 'calf_r': (-0.40, 0.0, 0.0), 'foot_r': (0.24, 0.0, 0.0),
           'thigh_l': (-0.55, 0.0, -0.04), 'calf_l': (-0.75, 0.0, 0.0), 'foot_l': (-0.20, 0.0, 0.0),
           'upperarm_l': (-1.150, 0.0, -0.80), 'upperarm_r': (-1.150, 0.0, -0.30),
           'lowerarm_l': (1.35, 0.0, 0.0), 'lowerarm_r': (0.95, 0.0, 0.0),
           'spine_01': (0.10, 0.0, 0.0), 'spine_02': (0.12, 0.0, 0.14),
           'neck_01': (-0.10, 0.0, 0.0)}),
    merge(STAND, pelvis(up=-0.075, side=0.018),
          {'thigh_r': (0.34, 0.0, 0.04), 'calf_r': (-0.72, 0.0, 0.0), 'foot_r': (0.02, 0.0, 0.0),
           'thigh_l': (-0.30, 0.0, -0.04), 'calf_l': (-1.10, 0.0, 0.0), 'foot_l': (0.10, 0.0, 0.0),
           'upperarm_l': (-1.150, 0.0, -0.50), 'upperarm_r': (-1.150, 0.0, -0.10),
           'lowerarm_l': (1.15, 0.0, 0.0), 'lowerarm_r': (1.05, 0.0, 0.0),
           'spine_01': (0.12, 0.0, 0.0), 'spine_02': (0.14, 0.0, 0.07),
           'neck_01': (-0.12, 0.0, 0.0)}),
    # vuelo: ambos pies fuera del suelo
    merge(STAND, pelvis(up=0.050, side=0.010),
          {'thigh_r': (-0.30, 0.0, 0.04), 'calf_r': (-0.30, 0.0, 0.0), 'foot_r': (-0.30, 0.0, 0.0),
           'thigh_l': (0.55, 0.0, -0.04), 'calf_l': (-1.55, 0.0, 0.0), 'foot_l': (0.25, 0.0, 0.0),
           'upperarm_l': (-1.150, 0.0, -0.15), 'upperarm_r': (-1.150, 0.0, 0.15),
           'lowerarm_l': (1.05, 0.0, 0.0), 'lowerarm_r': (1.05, 0.0, 0.0),
           'spine_01': (0.11, 0.0, 0.0), 'spine_02': (0.13, 0.0, -0.02),
           'neck_01': (-0.11, 0.0, 0.0)}),
    merge(STAND, pelvis(up=0.010),
          {'thigh_r': (-0.52, 0.0, 0.04), 'calf_r': (-0.55, 0.0, 0.0), 'foot_r': (-0.34, 0.0, 0.0),
           'thigh_l': (0.78, 0.0, -0.04), 'calf_l': (-1.05, 0.0, 0.0), 'foot_l': (0.20, 0.0, 0.0),
           'upperarm_l': (-1.150, 0.0, 0.25), 'upperarm_r': (-1.150, 0.0, 0.62),
           'lowerarm_l': (0.95, 0.0, 0.0), 'lowerarm_r': (1.25, 0.0, 0.0),
           'spine_01': (0.10, 0.0, 0.0), 'spine_02': (0.12, 0.0, -0.12),
           'neck_01': (-0.10, 0.0, 0.0)}),
]

CROUCH_BASE = merge(
    STAND, pelvis(up=-0.255, back=-0.115),
    {'thigh_l': (0.95, 0.0, -0.13), 'thigh_r': (0.95, 0.0, 0.13),
     'calf_l': (-1.62, 0.0, 0.0), 'calf_r': (-1.62, 0.0, 0.0),
     'foot_l': (0.62, 0.0, 0.0), 'foot_r': (0.62, 0.0, 0.0),
     'spine_01': (0.14, 0.0, 0.0), 'spine_02': (0.16, 0.0, 0.0),
     'spine_03': (0.06, 0.0, 0.0), 'neck_01': (-0.14, 0.0, 0.0),
     'upperarm_l': (-1.100, 0.0, -0.24), 'upperarm_r': (-1.100, 0.0, 0.24),
     'lowerarm_l': (0.72, 0.0, 0.0), 'lowerarm_r': (0.72, 0.0, 0.0)})

CROUCH_HALF = [
    merge(CROUCH_BASE,
          {'thigh_r': (1.16, 0.0, 0.13), 'calf_r': (-1.50, 0.0, 0.0), 'foot_r': (0.50, 0.0, 0.0),
           'thigh_l': (0.72, 0.0, -0.13), 'calf_l': (-1.66, 0.0, 0.0), 'foot_l': (0.40, 0.0, 0.0)}),
    merge(CROUCH_BASE, pelvis(up=-0.275, back=-0.115),
          {'thigh_r': (1.02, 0.0, 0.13), 'calf_r': (-1.60, 0.0, 0.0), 'foot_r': (0.58, 0.0, 0.0),
           'thigh_l': (0.86, 0.0, -0.13), 'calf_l': (-1.72, 0.0, 0.0), 'foot_l': (0.52, 0.0, 0.0)}),
    merge(CROUCH_BASE, pelvis(up=-0.245, back=-0.115),
          {'thigh_r': (0.86, 0.0, 0.13), 'calf_r': (-1.62, 0.0, 0.0), 'foot_r': (0.62, 0.0, 0.0),
           'thigh_l': (1.06, 0.0, -0.13), 'calf_l': (-1.90, 0.0, 0.0), 'foot_l': (0.68, 0.0, 0.0)}),
    merge(CROUCH_BASE, pelvis(up=-0.265, back=-0.115),
          {'thigh_r': (0.78, 0.0, 0.13), 'calf_r': (-1.58, 0.0, 0.0), 'foot_r': (0.46, 0.0, 0.0),
           'thigh_l': (1.14, 0.0, -0.13), 'calf_l': (-1.74, 0.0, 0.0), 'foot_l': (0.58, 0.0, 0.0)}),
]


def cycle_from_half(half):
    """Ciclo completo de 8 fases a partir del medio ciclo + su espejo."""
    return list(half) + [mirror(p) for p in half]


def reverse_cycle(cycle):
    """Marcha atras: mismo ciclo recorrido al reves, manteniendo la fase 0."""
    return [cycle[0]] + list(reversed(cycle[1:]))


def strafe_cycle(lead='l'):
    """Paso lateral hacia el lado `lead`; el otro pie sigue sin cruzarse."""
    trail = 'r' if lead == 'l' else 'l'
    s_lead = 1.0 if lead == 'l' else -1.0
    s_trail = -s_lead

    def key(open_lead, lift_trail, lift_lead, up):
        p = merge(STAND, pelvis(up=up, side=0.02 * s_lead))
        p['thigh_' + lead] = (0.04, 0.0, open_lead * s_lead)
        p['calf_' + lead] = (-0.10 - lift_lead, 0.0, 0.0)
        p['foot_' + lead] = (0.03 + lift_lead * 0.35, 0.0, 0.0)
        p['thigh_' + trail] = (0.04, 0.0, (0.07 + lift_trail * 0.30) * s_trail)
        p['calf_' + trail] = (-0.10 - lift_trail, 0.0, 0.0)
        p['foot_' + trail] = (0.03 + lift_trail * 0.35, 0.0, 0.0)
        p['upperarm_l'] = (-1.230, 0.0, -0.13)
        p['upperarm_r'] = (-1.230, 0.0, 0.13)
        p['spine_02'] = (0.02, 0.0, 0.05 * s_lead)
        return p

    return [key(0.34, 0.00, 0.00, -0.010),
            key(0.36, 0.30, 0.00, -0.026),
            key(0.20, 0.55, 0.00, -0.016),
            key(0.12, 0.18, 0.00, -0.030),
            key(0.10, 0.00, 0.00, -0.012),
            key(0.20, 0.00, 0.30, -0.028),
            key(0.34, 0.00, 0.52, -0.018),
            key(0.38, 0.00, 0.16, -0.030)]


# --------------------------------------------------------------- clips sueltos
def breath(amount, lift=0.006):
    return {'spine_01': (0.015 - amount * 0.5, 0.0, 0.0),
            'spine_02': (0.02 + amount, 0.0, 0.0),
            'spine_03': (0.01 + amount * 0.6, 0.0, 0.0),
            'neck_01': (0.02 - amount * 0.8, 0.0, 0.0),
            'pelvis': ('loc', 0.0, lift, 0.0)}


def reach(fraction, side='r', height=1.0):
    """Brazo que se extiende al frente; `fraction` 0 = reposo, 1 = extendido."""
    s = 1.0 if side == 'l' else -1.0
    return {'upperarm_' + side: (-1.245 + 0.62 * fraction * height, 0.0, -0.13 * s - 0.55 * fraction * s),
            'lowerarm_' + side: (0.30 + 0.55 * fraction, 0.0, 0.0),
            'clavicle_' + side: (0.0, 0.0, -0.10 * fraction * s)}


GRIPS = {
    'Grip_Open':    hands(0.0, thumb=0.0, spread=0.10),
    'Grip_Fist':    hands(1.32, thumb=0.85),
    'Grip_Pistol':  hands(1.05, thumb=0.55, index=0.55),
    'Grip_Rifle':   hands(1.10, thumb=0.60, index=0.50),
    'Grip_Tool':    hands(0.95, thumb=0.70),
    'Grip_Object':  hands(0.78, thumb=0.50, spread=0.06),
    'Grip_TwoHand': hands(1.12, thumb=0.65, index=0.52),
}

ONEHAND_HOLD = merge(STAND, hand('l', 0.26, thumb=0.30), hand('r', 1.05, thumb=0.55, index=0.55),
                     {'upperarm_r': (-1.020, 0.0, 0.28), 'lowerarm_r': (0.95, 0.0, 0.0)})
ONEHAND_AIM = merge(ONEHAND_HOLD,
                    {'upperarm_r': (-0.360, 0.0, 0.34), 'lowerarm_r': (0.34, 0.0, 0.0),
                     'upperarm_l': (-0.700, 0.0, -0.55), 'lowerarm_l': (1.05, 0.0, 0.0),
                     'spine_02': (0.03, 0.0, -0.10), 'neck_01': (0.04, 0.0, -0.06)},
                    hand('l', 0.85, thumb=0.55))
TWOHAND_HOLD = merge(STAND, hands(1.10, thumb=0.60, index=0.50),
                     {'upperarm_r': (-0.980, 0.0, 0.32), 'lowerarm_r': (1.15, 0.0, 0.0),
                      'upperarm_l': (-0.900, 0.0, -0.62), 'lowerarm_l': (1.30, 0.0, 0.0),
                      'spine_02': (0.02, 0.0, -0.08)})
TWOHAND_AIM = merge(TWOHAND_HOLD,
                    {'upperarm_r': (-0.560, 0.0, 0.46), 'lowerarm_r': (1.05, 0.0, 0.0),
                     'upperarm_l': (-0.560, 0.0, -0.78), 'lowerarm_l': (1.32, 0.0, 0.0),
                     'spine_02': (0.03, 0.0, -0.16), 'neck_01': (0.05, 0.0, -0.10),
                     'clavicle_r': (0.0, 0.0, 0.10)})
TOOL_HOLD = merge(STAND, hand('l', 0.26, thumb=0.30), hand('r', 0.95, thumb=0.70),
                  {'upperarm_r': (-0.880, 0.0, 0.36), 'lowerarm_r': (1.05, 0.0, 0.0),
                   'spine_02': (0.02, 0.0, -0.06)})


def build_clips():
    """Devuelve [(nombre, frames, loop, [(frame_norm, pose), ...])]."""
    clips = []

    def cyc(name, frames, cycle):
        keys = [(i / 8.0, cycle[i % 8]) for i in range(9)]
        clips.append((name, frames, True, keys))

    def seq(name, frames, loop, keys):
        clips.append((name, frames, loop, keys))

    # ------------------------------------------------------------ idle
    seq('AN_Player_Idle', 91, True, [
        (0.00, merge(STAND, breath(0.000, 0.000))),
        (0.28, merge(STAND, breath(0.028, 0.008), {'head': (-0.01, 0.0, 0.03)})),
        (0.50, merge(STAND, breath(0.006, 0.002), {'head': (-0.01, 0.0, 0.05)})),
        (0.74, merge(STAND, breath(0.030, 0.009), {'head': (-0.01, 0.0, 0.01)})),
        (1.00, merge(STAND, breath(0.000, 0.000))),
    ])

    # ------------------------------------------------------- locomocion
    walk = cycle_from_half(WALK_HALF)
    run = cycle_from_half(RUN_HALF)
    cyc('AN_Player_Walk_F', 37, walk)
    cyc('AN_Player_Walk_B', 37, reverse_cycle(walk))
    cyc('AN_Player_Walk_L', 37, strafe_cycle('l'))
    cyc('AN_Player_Walk_R', 37, strafe_cycle('r'))
    cyc('AN_Player_Run_F', 25, run)
    cyc('AN_Player_Run_B', 25, reverse_cycle(run))
    cyc('AN_Player_Run_L', 25, [merge(p, {'spine_01': (0.07, 0.0, 0.0)}) for p in strafe_cycle('l')])
    cyc('AN_Player_Run_R', 25, [merge(p, {'spine_01': (0.07, 0.0, 0.0)}) for p in strafe_cycle('r')])

    # ------------------------------------------------------------ salto
    crouch_load = merge(STAND, pelvis(up=-0.150, back=-0.060),
                        {'thigh_l': (0.62, 0.0, -0.09), 'thigh_r': (0.62, 0.0, 0.09),
                         'calf_l': (-1.05, 0.0, 0.0), 'calf_r': (-1.05, 0.0, 0.0),
                         'foot_l': (0.42, 0.0, 0.0), 'foot_r': (0.42, 0.0, 0.0),
                         'spine_02': (0.16, 0.0, 0.0), 'neck_01': (-0.12, 0.0, 0.0),
                         'upperarm_l': (-1.100, 0.0, 0.42), 'upperarm_r': (-1.100, 0.0, -0.42),
                         'lowerarm_l': (0.55, 0.0, 0.0), 'lowerarm_r': (0.55, 0.0, 0.0)})
    launch = merge(STAND, pelvis(up=0.035),
                   {'thigh_l': (-0.10, 0.0, -0.05), 'thigh_r': (-0.10, 0.0, 0.05),
                    'calf_l': (-0.05, 0.0, 0.0), 'calf_r': (-0.05, 0.0, 0.0),
                    'foot_l': (-0.55, 0.0, 0.0), 'foot_r': (-0.55, 0.0, 0.0),
                    'spine_02': (-0.04, 0.0, 0.0),
                    'upperarm_l': (-0.480, 0.0, -0.42), 'upperarm_r': (-0.480, 0.0, 0.42),
                    'lowerarm_l': (0.30, 0.0, 0.0), 'lowerarm_r': (0.30, 0.0, 0.0)})
    airborne = merge(STAND, pelvis(up=-0.010),
                     {'thigh_l': (0.38, 0.0, -0.10), 'thigh_r': (0.24, 0.0, 0.10),
                      'calf_l': (-0.72, 0.0, 0.0), 'calf_r': (-0.48, 0.0, 0.0),
                      'foot_l': (-0.18, 0.0, 0.0), 'foot_r': (-0.24, 0.0, 0.0),
                      'spine_02': (0.06, 0.0, 0.0),
                      'upperarm_l': (-0.820, 0.0, -0.30), 'upperarm_r': (-0.820, 0.0, 0.30),
                      'lowerarm_l': (0.72, 0.0, 0.0), 'lowerarm_r': (0.72, 0.0, 0.0)})
    seq('AN_Player_Jump_Start', 16, False, [
        (0.00, STAND), (0.45, crouch_load), (0.80, launch),
        (1.00, merge(launch, pelvis(up=0.045), {'thigh_l': (0.10, 0.0, -0.05),
                                                'thigh_r': (0.10, 0.0, 0.05)})),
    ])
    seq('AN_Player_Jump_Loop', 31, True, [
        (0.00, airborne),
        (0.50, merge(airborne, pelvis(up=0.004),
                     {'thigh_l': (0.30, 0.0, -0.10), 'calf_l': (-0.62, 0.0, 0.0),
                      'thigh_r': (0.32, 0.0, 0.10), 'calf_r': (-0.58, 0.0, 0.0),
                      'upperarm_l': (-0.880, 0.0, -0.26), 'upperarm_r': (-0.880, 0.0, 0.26)})),
        (1.00, airborne),
    ])
    impact = merge(STAND, pelvis(up=-0.215, back=-0.090),
                   {'thigh_l': (0.86, 0.0, -0.12), 'thigh_r': (0.86, 0.0, 0.12),
                    'calf_l': (-1.42, 0.0, 0.0), 'calf_r': (-1.42, 0.0, 0.0),
                    'foot_l': (0.56, 0.0, 0.0), 'foot_r': (0.56, 0.0, 0.0),
                    'spine_02': (0.24, 0.0, 0.0), 'neck_01': (-0.16, 0.0, 0.0),
                    'upperarm_l': (-0.760, 0.0, -0.50), 'upperarm_r': (-0.760, 0.0, 0.50),
                    'lowerarm_l': (0.95, 0.0, 0.0), 'lowerarm_r': (0.95, 0.0, 0.0)})
    seq('AN_Player_Jump_Land', 22, False, [
        (0.00, merge(airborne, {'foot_l': (0.28, 0.0, 0.0), 'foot_r': (0.28, 0.0, 0.0)})),
        (0.26, impact),
        (0.62, merge(STAND, pelvis(up=-0.055, back=-0.020),
                     {'thigh_l': (0.28, 0.0, -0.07), 'thigh_r': (0.28, 0.0, 0.07),
                      'calf_l': (-0.50, 0.0, 0.0), 'calf_r': (-0.50, 0.0, 0.0),
                      'foot_l': (0.22, 0.0, 0.0), 'foot_r': (0.22, 0.0, 0.0),
                      'spine_02': (0.08, 0.0, 0.0)})),
        (1.00, STAND),
    ])

    # --------------------------------------------------------- agachado
    seq('AN_Player_Crouch_Enter', 22, False, [
        (0.00, STAND), (0.55, merge(CROUCH_BASE, pelvis(up=-0.290, back=-0.125))),
        (1.00, CROUCH_BASE)])
    seq('AN_Player_Crouch_Idle', 61, True, [
        (0.00, CROUCH_BASE),
        (0.34, merge(CROUCH_BASE, pelvis(up=-0.247, back=-0.115), {'spine_02': (0.19, 0.0, 0.0)})),
        (0.68, merge(CROUCH_BASE, pelvis(up=-0.259, back=-0.115), {'spine_02': (0.15, 0.0, 0.0)})),
        (1.00, CROUCH_BASE)])
    crouch = cycle_from_half(CROUCH_HALF)
    cyc('AN_Player_Crouch_Walk_F', 43, crouch)
    cyc('AN_Player_Crouch_Walk_B', 43, reverse_cycle(crouch))
    def crouched(p):
        """Aplica el offset de agachado sobre una pose de paso lateral."""
        out = merge(p, CROUCH_BASE)
        for s in ('l', 'r'):
            th = p['thigh_' + s]
            cf = p['calf_' + s]
            ft = p['foot_' + s]
            out['thigh_' + s] = (th[0] + 0.93, th[1], th[2] * 1.6)
            out['calf_' + s] = (cf[0] - 1.52, cf[1], cf[2])
            out['foot_' + s] = (ft[0] + 0.56, ft[1], ft[2])
        base = p.get('pelvis', ('loc', 0.0, 0.0, 0.0))
        out['pelvis'] = ('loc', base[1], base[2] - 0.245, -0.115)
        return out

    cyc('AN_Player_Crouch_Walk_L', 43, [crouched(p) for p in strafe_cycle('l')])
    cyc('AN_Player_Crouch_Walk_R', 43, [crouched(p) for p in strafe_cycle('r')])
    seq('AN_Player_Crouch_Exit', 22, False, [
        (0.00, CROUCH_BASE), (0.50, merge(STAND, pelvis(up=-0.090, back=-0.040),
                                          {'thigh_l': (0.42, 0.0, -0.10), 'thigh_r': (0.42, 0.0, 0.10),
                                           'calf_l': (-0.78, 0.0, 0.0), 'calf_r': (-0.78, 0.0, 0.0),
                                           'foot_l': (0.32, 0.0, 0.0), 'foot_r': (0.32, 0.0, 0.0),
                                           'spine_02': (0.10, 0.0, 0.0)})),
        (1.00, STAND)])

    # ------------------------------------------------------- interaccion
    seq('AN_Player_Interact', 46, False, [
        (0.00, STAND),
        (0.35, merge(STAND, reach(0.85), hand('r', 0.30, thumb=0.20),
                     {'spine_02': (0.05, 0.0, -0.10), 'neck_01': (0.06, 0.0, -0.08)})),
        (0.52, merge(STAND, reach(0.95), hand('r', 0.75, thumb=0.55),
                     {'spine_02': (0.06, 0.0, -0.12), 'neck_01': (0.07, 0.0, -0.09)})),
        (0.72, merge(STAND, reach(0.80), hand('r', 0.40, thumb=0.30),
                     {'spine_02': (0.05, 0.0, -0.10)})),
        (1.00, STAND)])
    seq('AN_Player_Pickup', 61, False, [
        (0.00, STAND),
        (0.34, merge(STAND, pelvis(up=-0.190, back=-0.080), reach(0.55, height=0.35),
                     {'thigh_l': (0.80, 0.0, -0.12), 'thigh_r': (0.80, 0.0, 0.12),
                      'calf_l': (-1.32, 0.0, 0.0), 'calf_r': (-1.32, 0.0, 0.0),
                      'foot_l': (0.52, 0.0, 0.0), 'foot_r': (0.52, 0.0, 0.0),
                      'spine_02': (0.34, 0.0, 0.0), 'neck_01': (0.10, 0.0, 0.0)},
                     hand('r', 0.20, thumb=0.15))),
        (0.48, merge(STAND, pelvis(up=-0.205, back=-0.085), reach(0.60, height=0.30),
                     {'thigh_l': (0.86, 0.0, -0.12), 'thigh_r': (0.86, 0.0, 0.12),
                      'calf_l': (-1.40, 0.0, 0.0), 'calf_r': (-1.40, 0.0, 0.0),
                      'foot_l': (0.56, 0.0, 0.0), 'foot_r': (0.56, 0.0, 0.0),
                      'spine_02': (0.38, 0.0, 0.0), 'neck_01': (0.12, 0.0, 0.0)},
                     hand('r', 0.80, thumb=0.55))),
        (0.78, merge(STAND, reach(0.45, height=0.70),
                     {'spine_02': (0.08, 0.0, 0.0)}, hand('r', 0.80, thumb=0.55))),
        (1.00, merge(STAND, hand('r', 0.78, thumb=0.50)))])
    seq('AN_Player_UseTool', 46, False, [
        (0.00, TOOL_HOLD),
        (0.30, merge(TOOL_HOLD, {'upperarm_r': (-0.560, 0.0, 0.40), 'lowerarm_r': (1.20, 0.0, 0.0),
                                 'neck_01': (0.06, 0.0, -0.08)})),
        (0.55, merge(TOOL_HOLD, {'upperarm_r': (-0.420, 0.0, 0.44), 'lowerarm_r': (0.85, 0.0, 0.0),
                                 'spine_02': (0.03, 0.0, -0.12), 'neck_01': (0.07, 0.0, -0.10)})),
        (0.78, merge(TOOL_HOLD, {'upperarm_r': (-0.600, 0.0, 0.40), 'lowerarm_r': (1.15, 0.0, 0.0)})),
        (1.00, TOOL_HOLD)])
    scan_pose = merge(STAND, hand('l', 0.30, thumb=0.25), hand('r', 0.55, thumb=0.45),
                      {'upperarm_l': (-0.620, 0.0, -0.62), 'lowerarm_l': (1.42, 0.0, 0.0),
                       'clavicle_l': (0.0, 0.0, -0.08), 'neck_01': (0.10, 0.0, 0.10),
                       'head': (0.04, 0.0, 0.08)})
    seq('AN_Player_Scan', 76, False, [
        (0.00, STAND),
        (0.22, scan_pose),
        (0.48, merge(scan_pose, {'spine_02': (0.02, 0.0, -0.18), 'neck_01': (0.10, 0.0, -0.08),
                                 'head': (0.04, 0.0, -0.06)})),
        (0.74, merge(scan_pose, {'spine_02': (0.02, 0.0, 0.16), 'neck_01': (0.10, 0.0, 0.16),
                                 'head': (0.04, 0.0, 0.10)})),
        (0.90, scan_pose),
        (1.00, STAND)])
    inspect_pose = merge(STAND, hand('r', 0.72, thumb=0.55), hand('l', 0.30, thumb=0.25),
                         {'upperarm_r': (-0.520, 0.0, 0.50), 'lowerarm_r': (1.50, 0.0, 0.0),
                          'neck_01': (0.12, 0.0, -0.10), 'head': (0.06, 0.0, -0.08),
                          'spine_02': (0.04, 0.0, -0.06)})
    seq('AN_Player_Inspect', 91, False, [
        (0.00, STAND),
        (0.20, inspect_pose),
        (0.45, merge(inspect_pose, {'lowerarm_r': (1.62, 0.0, 0.0), 'hand_r': (0.0, 0.55, 0.0)})),
        (0.70, merge(inspect_pose, {'lowerarm_r': (1.44, 0.0, 0.0), 'hand_r': (0.0, -0.45, 0.0)})),
        (0.88, inspect_pose),
        (1.00, STAND)])

    # --------------------------------------------------- armas / herramienta
    def equip_set(prefix, hold, aim, draw_from, frames_e, frames_i, frames_a):
        seq('AN_Player_%s_Equip' % prefix, frames_e, False, [
            (0.00, STAND), (0.40, draw_from), (0.75, merge(hold, {'spine_02': (0.02, 0.0, -0.12)})),
            (1.00, hold)])
        seq('AN_Player_%s_Idle' % prefix, frames_i, True, [
            (0.00, hold),
            (0.35, merge(hold, breath(0.022, 0.006))),
            (0.70, merge(hold, breath(0.006, 0.002))),
            (1.00, hold)])
        seq('AN_Player_%s_Aim' % prefix, frames_a, True, [
            (0.00, aim),
            (0.50, merge(aim, breath(0.010, 0.003))),
            (1.00, aim)])
        seq('AN_Player_%s_Lower' % prefix, frames_a, False, [
            (0.00, aim), (0.60, merge(hold, {'spine_02': (0.02, 0.0, -0.10)})), (1.00, hold)])
        seq('AN_Player_%s_Unequip' % prefix, frames_e, False, [
            (0.00, hold), (0.35, draw_from), (1.00, STAND)])

    draw_hip = merge(STAND, hand('r', 0.85, thumb=0.55),
                     {'upperarm_r': (-1.320, 0.0, 0.10), 'lowerarm_r': (0.62, 0.0, 0.0),
                      'spine_02': (0.03, 0.0, 0.10)})
    draw_back = merge(STAND, hand('r', 0.85, thumb=0.55),
                      {'upperarm_r': (0.180, 0.0, 0.30), 'lowerarm_r': (1.65, 0.0, 0.0),
                       'clavicle_r': (0.06, 0.0, 0.10), 'spine_02': (0.02, 0.0, 0.14)})
    equip_set('OneHand', ONEHAND_HOLD, ONEHAND_AIM, draw_hip, 31, 61, 31)
    equip_set('TwoHand', TWOHAND_HOLD, TWOHAND_AIM, draw_back, 31, 61, 31)

    seq('AN_Player_Tool_Equip', 46, False, [
        (0.00, STAND), (0.40, draw_hip), (1.00, TOOL_HOLD)])
    seq('AN_Player_Tool_Idle', 61, True, [
        (0.00, TOOL_HOLD), (0.35, merge(TOOL_HOLD, breath(0.020, 0.006))),
        (0.70, merge(TOOL_HOLD, breath(0.006, 0.002))), (1.00, TOOL_HOLD)])
    seq('AN_Player_Tool_Use', 46, False, [
        (0.00, TOOL_HOLD),
        (0.28, merge(TOOL_HOLD, {'upperarm_r': (-0.520, 0.0, 0.42), 'lowerarm_r': (1.28, 0.0, 0.0)})),
        (0.52, merge(TOOL_HOLD, {'upperarm_r': (-0.400, 0.0, 0.46), 'lowerarm_r': (0.80, 0.0, 0.0),
                                 'spine_02': (0.04, 0.0, -0.14)})),
        (0.76, merge(TOOL_HOLD, {'upperarm_r': (-0.560, 0.0, 0.42), 'lowerarm_r': (1.18, 0.0, 0.0)})),
        (1.00, TOOL_HOLD)])
    seq('AN_Player_Tool_Unequip', 46, False, [
        (0.00, TOOL_HOLD), (0.40, draw_hip), (1.00, STAND)])

    # ------------------------------------------- gestos de herramienta (FP)
    # El rig de primera persona (UAstraeonFirstPersonRigComponent) resuelve nueve
    # gestos. Ocho ya salen de los clips de arriba; estos seis faltaban y sin
    # ellos el protagonista no puede reemplazar al blockout humano.
    # Los seis se derivan de poses ya auditadas —TWOHAND_HOLD para lo que se
    # empuña a dos manos, TOOL_HOLD para lo de una— y solo varían la elevación
    # del hombro y la flexión del codo. Inventar los ángulos desde cero daba
    # brazos abiertos en cruz: el cruce al frente lo fija Z, negativo en el brazo
    # izquierdo y positivo en el derecho, y hay que conservarlo.
    pulse_ready = merge(TWOHAND_HOLD,
                        {'upperarm_r': (-0.700, 0.0, 0.34), 'lowerarm_r': (1.08, 0.0, 0.0),
                         'upperarm_l': (-0.660, 0.0, -0.66), 'lowerarm_l': (1.24, 0.0, 0.0),
                         'spine_02': (0.03, 0.0, -0.14), 'neck_01': (0.05, 0.0, -0.08)})
    pulse_kick = merge(pulse_ready,
                       {'lowerarm_r': (1.22, 0.0, 0.0), 'lowerarm_l': (1.38, 0.0, 0.0),
                        'spine_02': (0.03, 0.0, -0.05)})
    seq('AN_Player_Pulse', 46, False, [
        (0.00, TOOL_HOLD), (0.20, pulse_ready), (0.40, pulse_kick), (0.52, pulse_ready),
        (0.70, pulse_kick), (0.84, pulse_ready), (1.00, TOOL_HOLD)])

    drill_press = merge(TWOHAND_HOLD,
                        {'upperarm_r': (-0.780, 0.0, 0.34), 'lowerarm_r': (0.98, 0.0, 0.0),
                         'upperarm_l': (-0.740, 0.0, -0.64), 'lowerarm_l': (1.18, 0.0, 0.0),
                         'spine_02': (0.05, 0.0, -0.20), 'neck_01': (0.10, 0.0, -0.14),
                         'head': (0.05, 0.0, -0.10)})
    drill_bite = merge(drill_press,
                       {'lowerarm_r': (0.86, 0.0, 0.0), 'lowerarm_l': (1.06, 0.0, 0.0),
                        'spine_02': (0.05, 0.0, -0.25)})
    seq('AN_Player_Drill', 61, False, [
        (0.00, TOOL_HOLD), (0.18, drill_press), (0.34, drill_bite), (0.48, drill_press),
        (0.62, drill_bite), (0.76, drill_press), (0.90, drill_bite), (1.00, TOOL_HOLD)])

    hammer_up = merge(TOOL_HOLD,
                      {'upperarm_r': (-0.220, 0.0, 0.40), 'lowerarm_r': (1.78, 0.0, 0.0),
                       'clavicle_r': (0.05, 0.0, 0.10), 'spine_02': (0.02, 0.0, 0.16),
                       'neck_01': (0.02, 0.0, 0.06)})
    hammer_hit = merge(TOOL_HOLD,
                       {'upperarm_r': (-1.020, 0.0, 0.42), 'lowerarm_r': (0.48, 0.0, 0.0),
                        'spine_02': (0.05, 0.0, -0.24), 'neck_01': (0.10, 0.0, -0.14),
                        'head': (0.05, 0.0, -0.10)})
    seq('AN_Player_Hammer', 41, False, [
        (0.00, TOOL_HOLD), (0.30, hammer_up), (0.52, hammer_hit),
        (0.66, merge(hammer_hit, {'lowerarm_r': (0.76, 0.0, 0.0)})), (1.00, TOOL_HOLD)])

    maul_up = merge(TWOHAND_HOLD,
                    {'upperarm_r': (-0.240, 0.0, 0.34), 'lowerarm_r': (1.80, 0.0, 0.0),
                     'upperarm_l': (-0.200, 0.0, -0.60), 'lowerarm_l': (1.86, 0.0, 0.0),
                     'clavicle_r': (0.05, 0.0, 0.08), 'spine_02': (0.02, 0.0, 0.20),
                     'neck_01': (0.02, 0.0, 0.10)})
    maul_hit = merge(TWOHAND_HOLD,
                     {'upperarm_r': (-1.080, 0.0, 0.30), 'lowerarm_r': (0.44, 0.0, 0.0),
                      'upperarm_l': (-1.020, 0.0, -0.58), 'lowerarm_l': (0.56, 0.0, 0.0),
                      'spine_02': (0.06, 0.0, -0.32), 'neck_01': (0.12, 0.0, -0.16),
                      'head': (0.06, 0.0, -0.12)})
    seq('AN_Player_Maul', 51, False, [
        (0.00, TOOL_HOLD), (0.16, TWOHAND_HOLD), (0.42, maul_up), (0.62, maul_hit),
        (0.76, merge(maul_hit, {'lowerarm_r': (0.70, 0.0, 0.0), 'lowerarm_l': (0.82, 0.0, 0.0)})),
        (0.90, TWOHAND_HOLD), (1.00, TOOL_HOLD)])

    # A la boca: se parte de la elevación de `Inspect`, que ya lleva la mano a la
    # cara, y se cierra más el codo. Z mayor cruza la mano hacia la línea media.
    consume_hold = merge(STAND, hand('r', 0.80, thumb=0.55), hand('l', 0.25, thumb=0.22),
                         {'upperarm_r': (-0.860, 0.0, 0.38), 'lowerarm_r': (1.24, 0.0, 0.0)})
    consume_mouth = merge(consume_hold,
                          {'upperarm_r': (-0.480, 0.0, 0.58), 'lowerarm_r': (1.92, 0.0, 0.0),
                           'hand_r': (0.0, 0.30, 0.0), 'clavicle_r': (0.02, 0.0, 0.06),
                           'neck_01': (-0.02, 0.0, 0.04), 'head': (-0.02, 0.0, 0.05)})
    seq('AN_Player_Consume', 61, False, [
        (0.00, STAND), (0.18, consume_hold), (0.40, consume_mouth),
        (0.56, merge(consume_mouth, {'head': (0.03, 0.0, 0.02)})),
        (0.70, consume_mouth), (0.86, consume_hold), (1.00, STAND)])

    # Ofrecer: brazo casi extendido al frente, palma arriba, no cruzado al pecho.
    present_pose = merge(STAND, hand('r', 0.35, thumb=0.30, spread=0.08),
                         hand('l', 0.28, thumb=0.24),
                         {'upperarm_r': (-0.560, 0.0, 0.30), 'lowerarm_r': (0.62, 0.0, 0.0),
                          'hand_r': (0.0, -0.55, 0.0), 'clavicle_r': (0.02, 0.0, 0.06),
                          'spine_02': (0.02, 0.0, -0.06),
                          'neck_01': (0.03, 0.0, -0.04), 'head': (0.02, 0.0, -0.04)})
    seq('AN_Player_Present', 76, False, [
        (0.00, STAND), (0.22, present_pose),
        (0.52, merge(present_pose, breath(0.020, 0.006))),
        (0.80, present_pose), (1.00, STAND)])

    # ------------------------------------------------------ poses de agarre
    for name, pose in GRIPS.items():
        seq(name, 1, False, [(0.0, merge(STAND, pose))])

    return clips


# Gestos que el rig de primera persona necesita y que no existian en el lote
# original de 45. Se listan aparte para poder autorizarlos sin regenerar —ni
# volver a asentar sobre el suelo— los clips que ya estaban auditados.
FIRST_PERSON_GESTURES = ('AN_Player_Pulse', 'AN_Player_Drill', 'AN_Player_Hammer',
                         'AN_Player_Maul', 'AN_Player_Consume', 'AN_Player_Present')


# ---------------------------------------------------------------- escritura
def action_fcurves(act):
    """F-curves de una Action. Blender 5.x usa Actions con slots/capas y ya no
    expone `Action.fcurves`; se conserva la ruta antigua para versiones previas."""
    legacy = getattr(act, 'fcurves', None)
    if legacy is not None:
        return list(legacy)
    out = []
    for layer in act.layers:
        for strip in layer.strips:
            for slot in act.slots:
                bag = strip.channelbag(slot)
                if bag:
                    out.extend(bag.fcurves)
    return out


def apply_pose(rig, pose):
    for pb in rig.pose.bones:
        pb.rotation_mode = 'XYZ'
        pb.rotation_euler = (0.0, 0.0, 0.0)
        pb.location = (0.0, 0.0, 0.0)
    for bone, val in pose.items():
        pb = rig.pose.bones.get(bone)
        if pb is None:
            continue
        if len(val) == 4 and val[0] == 'loc':
            pb.location = (val[1], val[2], val[3])
        else:
            pb.rotation_euler = val


def key_pose(rig, frame, pose):
    """Inserta claves solo en los canales que la pose realmente define."""
    apply_pose(rig, pose)
    for bone, val in pose.items():
        pb = rig.pose.bones.get(bone)
        if pb is None:
            continue
        if len(val) == 4 and val[0] == 'loc':
            pb.keyframe_insert('location', frame=frame, group=bone)
        else:
            pb.keyframe_insert('rotation_euler', frame=frame, group=bone)


def author_all(rig=None, report=True, only=None):
    """Escribe las Actions. `only` limita a una lista de nombres.

    `ground_clips` solo levanta la pelvis (`max(0, ...)`), asi que volver a
    pasarlo sobre clips ya asentados no los mueve; por eso basta con autorizar el
    subconjunto nuevo y reasentar todo.
    """
    rig = rig or bpy.data.objects[RIG_NAME]
    wanted = set(only) if only else None
    if bpy.context.object is not rig:
        bpy.ops.object.mode_set(mode='OBJECT')
        bpy.ops.object.select_all(action='DESELECT')
        rig.select_set(True)
        bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode='POSE')

    if rig.animation_data is None:
        rig.animation_data_create()

    made = []
    for name, frames, loop, keys in build_clips():
        if wanted is not None and name not in wanted:
            continue
        old = bpy.data.actions.get(name)
        if old:
            bpy.data.actions.remove(old)
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        rig.animation_data.action = act
        for norm, pose in keys:
            frame = 1 + int(round(norm * (frames - 1)))
            key_pose(rig, frame, pose)
        curves = action_fcurves(act)
        for fc in curves:
            for kp in fc.keyframe_points:
                kp.interpolation = 'BEZIER'
                kp.handle_left_type = kp.handle_right_type = 'AUTO_CLAMPED'
            fc.update()
        act.frame_start, act.frame_end = 1, frames
        act.use_frame_range = True
        act.use_cyclic = loop
        act['astraeon_loop'] = loop
        act['astraeon_fps'] = FPS
        made.append({'name': name, 'frames': frames, 'loop': loop,
                     'fcurves': len(curves),
                     'keys': len(curves[0].keyframe_points) if curves else 0})
    rig.animation_data.action = None
    bpy.ops.object.mode_set(mode='OBJECT')
    return made


# --------------------------------------------------- asentado de pies (suelo)
# Puntos de sonda en espacio de reposo, rigidamente unidos al hueso indicado.
GROUND_PROBES = [
    ('ball_l', (0.215, -0.178, 0.018)), ('ball_r', (-0.215, -0.178, 0.018)),
    ('foot_l', (0.215, 0.112, 0.018)), ('foot_r', (-0.215, 0.112, 0.018)),
    ('foot_l', (0.215, -0.030, 0.014)), ('foot_r', (-0.215, -0.030, 0.014)),
]
GROUND_Z = 0.012
AIRBORNE = ('AN_Player_Jump_Start', 'AN_Player_Jump_Loop')


def lowest_contact(rig):
    """Z minimo de las sondas de suela con la pose evaluada actual."""
    dg = bpy.context.evaluated_depsgraph_get()
    ev = rig.evaluated_get(dg)
    lo = None
    for bone, p in GROUND_PROBES:
        pb = ev.pose.bones[bone]
        rest_inv = ev.data.bones[bone].matrix_local.inverted()
        world = ev.matrix_world @ pb.matrix @ rest_inv @ Vector(p)
        lo = world.z if lo is None else min(lo, world.z)
    return lo


def ground_clips(rig=None):
    """Sube la pelvis lo justo para que ninguna suela atraviese el suelo.

    Solo corrige hacia arriba: conserva la fase aerea de carrera y salto, y la
    bajada del ciclo hasta donde la geometria de la pierna lo permite.
    """
    rig = rig or bpy.data.objects[RIG_NAME]
    if rig.animation_data is None:
        rig.animation_data_create()
    scene = bpy.context.scene
    fixed = []
    for act in bpy.data.actions:
        if not act.name.startswith('AN_Player_') or act.name in AIRBORNE:
            continue
        rig.animation_data.action = act
        curve = None
        for fc in action_fcurves(act):
            if fc.data_path == 'pose.bones["pelvis"].location' and fc.array_index == 1:
                curve = fc
                break
        if curve is None:
            continue
        before = []
        deltas = []
        for kp in curve.keyframe_points:
            frame = int(round(kp.co[0]))
            scene.frame_set(frame)
            lo = lowest_contact(rig)
            before.append(lo)
            deltas.append(max(0.0, GROUND_Z - lo))
        for kp, d in zip(curve.keyframe_points, deltas):
            kp.co[1] += d
            kp.handle_left[1] += d
            kp.handle_right[1] += d
        curve.update()
        fixed.append({'action': act.name,
                      'min_before_mm': round(min(before) * 1000, 1),
                      'max_lift_mm': round(max(deltas) * 1000, 1)})
    rig.animation_data.action = None
    scene.frame_set(1)
    return fixed


def _pelvis_up_curve(act):
    for fc in action_fcurves(act):
        if fc.data_path == 'pose.bones["pelvis"].location' and fc.array_index == 1:
            return fc
    return None


def ground_dense(rig, act):
    """Hornea la altura de pelvis fotograma a fotograma, solo hacia arriba.

    El pase por claves no basta cuando entre dos claves las piernas se pliegan
    mucho: la interpolacion Bezier de la pelvis no sigue a la geometria.
    """
    scene = bpy.context.scene
    rig.animation_data.action = act
    curve = _pelvis_up_curve(act)
    if curve is None:
        return None
    n = int(act.frame_end)
    plan = []
    worst = 0.0
    for f in range(1, n + 1):
        scene.frame_set(f)
        lo = lowest_contact(rig)
        worst = min(worst, lo)
        plan.append((f, curve.evaluate(f) + max(0.0, GROUND_Z - lo)))
    for f, v in plan:
        curve.keyframe_points.insert(f, v, options={'FAST'})
    for kp in curve.keyframe_points:
        kp.interpolation = 'BEZIER'
        kp.handle_left_type = kp.handle_right_type = 'AUTO_CLAMPED'
    curve.update()
    return round(worst * 1000, 1)


def ground_uniform(rig, act):
    """Desplaza la pelvis una constante: para clips integramente aereos, donde
    hornear por fotograma aplanaria el arco del salto."""
    scene = bpy.context.scene
    rig.animation_data.action = act
    curve = _pelvis_up_curve(act)
    if curve is None:
        return None
    lo = min(_sample_lows(rig, act, scene))
    d = max(0.0, GROUND_Z - lo)
    for kp in curve.keyframe_points:
        kp.co[1] += d
        kp.handle_left[1] += d
        kp.handle_right[1] += d
    curve.update()
    return round(d * 1000, 1)


def _sample_lows(rig, act, scene):
    return [(_set_and_probe(rig, scene, f)) for f in range(1, int(act.frame_end) + 1)]


def _set_and_probe(rig, scene, f):
    scene.frame_set(f)
    return lowest_contact(rig)


def ground_repair(rig=None, tolerance=-0.004):
    """Segunda pasada: corrige los clips que siguen penetrando el suelo."""
    rig = rig or bpy.data.objects[RIG_NAME]
    if rig.animation_data is None:
        rig.animation_data_create()
    scene = bpy.context.scene
    out = []
    for act in bpy.data.actions:
        if not act.name.startswith('AN_Player_'):
            continue
        rig.animation_data.action = act
        lows = _sample_lows(rig, act, scene)
        if min(lows) >= tolerance:
            continue
        if act.name == 'AN_Player_Jump_Loop':
            out.append({'action': act.name, 'mode': 'uniform',
                        'before_mm': round(min(lows) * 1000, 1),
                        'lift_mm': ground_uniform(rig, act)})
        else:
            out.append({'action': act.name, 'mode': 'dense',
                        'before_mm': ground_dense(rig, act)})
    rig.animation_data.action = None
    scene.frame_set(1)
    return out


if __name__ == '__main__':
    author_all()
    ground_clips()
    ground_repair()
