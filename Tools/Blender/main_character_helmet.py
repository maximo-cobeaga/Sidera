"""Casco desmontable del personaje principal de ASTRAEON.

La generacion 3D se pidio explicitamente SIN casco para conservar el rostro
visible, asi que el casco se construye aqui de forma parametrica sobre las
medidas reales de la cabeza (z 1.63-1.83, semiancho 0.095, profundidad 0.216).

Es un objeto independiente `SK_Astraeon_Helmet`, con cuatro slots de material
(Shell / Trim / Glass / Emission) y ponderado al hueso `head`, de modo que
Unreal pueda alternar su visibilidad sin tocar la cabeza del personaje.
"""
import math
import bpy
import bmesh
from mathutils import Vector

NAME = 'SK_Astraeon_Helmet'
COLLECTION = 'ASTRAEON_HELMET'

# Cabeza medida: semiancho maximo 0.093, semiprofundidad 0.122, corona 1.830,
# menton ~1.660. Radios elegidos dejan 15-20 mm de holgura en todo el contorno.
CENTRE = Vector((0.0, -0.012, 1.712))
RADII = Vector((0.116, 0.158, 0.136))
CROWN_TAPER = 0.16               # estrecha la corona: sin esto lee como bola
SHELL_THICKNESS = 0.009
TRIM_WIDTH = 0.013               # ancho del marco alrededor de la abertura
CHIN_CUT = 1.600                 # por debajo del menton (~1.660)
VISOR_FRONT = -0.56              # d.y por debajo del cual empieza la abertura
VISOR_LOW, VISOR_HIGH = -0.74, 0.60

MATERIALS = [
    # nombre, base_color, roughness, metallic, emission_rgb
    ('MAT_Player_HelmetShell', (0.855, 0.850, 0.825, 1.0), 0.32, 0.0, (0, 0, 0)),
    ('MAT_Player_HelmetTrim', (0.125, 0.135, 0.155, 1.0), 0.34, 0.70, (0, 0, 0)),
    # visor tintado opaco, no refractivo: en EEVEE la transmision sin trazado de
    # rayos aclara el cristal y el visor pierde el aspecto oscuro pedido.
    ('MAT_Player_HelmetGlass', (0.013, 0.017, 0.024, 1.0), 0.06, 0.0, (0, 0, 0)),
    ('MAT_Player_HelmetEmission', (0.02, 0.10, 0.16, 1.0), 0.25, 0.0, (0.06, 0.55, 0.90)),
]
SHELL, TRIM, GLASS, EMISSION = range(4)


def socket(node, identifier):
    """Entrada de un nodo por identificador estable (los nombres se traducen)."""
    for s in node.inputs:
        if s.identifier == identifier:
            return s
    return node.inputs.get(identifier)


def ensure_materials():
    out = []
    for name, base, rough, metal, emit in MATERIALS:
        mat = bpy.data.materials.get(name)
        if mat is None:
            mat = bpy.data.materials.new(name)
        mat.use_nodes = True
        # Buscar por tipo, nunca por nombre: en una instalacion localizada los
        # nodos se crean con el nombre traducido y `nodes.get('Principled BSDF')`
        # devuelve None, descartando todos los ajustes en silencio.
        bsdf = next((n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
        if bsdf:
            def put(ident, value):
                s = socket(bsdf, ident)
                if s is not None:
                    s.default_value = value

            put('Base Color', base)
            put('Roughness', rough)
            put('Metallic', metal)
            put('Emission Color', (*emit, 1.0))
            put('Emission Strength', 6.0 if any(emit) else 0.0)
            put('Transmission Weight', 0.0)
            if name.endswith('Glass'):
                put('IOR', 1.50)
                put('Coat Weight', 0.6)
                put('Coat Roughness', 0.04)
        out.append(mat)
    return out


def _shape(unit, scale=1.0):
    """Punto de la esfera unitaria -> superficie del casco, con corona ahusada."""
    t = 1.0 - CROWN_TAPER * max(0.0, unit.z) ** 2
    return Vector((unit.x * RADII.x * t * scale,
                   unit.y * RADII.y * t * scale,
                   unit.z * RADII.z * scale)) + CENTRE


def _norm(v):
    return Vector(((v.x - CENTRE.x) / RADII.x,
                   (v.y - CENTRE.y) / RADII.y,
                   (v.z - CENTRE.z) / RADII.z))


# La abertura del visor es la interseccion de tres semiespacios planos, asi que
# puede cortarse exactamente con bisect en vez de borrar caras enteras (que
# dejaba un borde escalonado siguiendo la malla de la esfera).
VISOR_Y = CENTRE.y + VISOR_FRONT * RADII.y
VISOR_Z0 = CENTRE.z + VISOR_LOW * RADII.z
VISOR_Z1 = CENTRE.z + VISOR_HIGH * RADII.z
EPS = 1e-5


def _in_visor(v, margin=0.0):
    return (v.y < VISOR_Y + margin
            and VISOR_Z0 - margin < v.z < VISOR_Z1 + margin)


def _below_jaw(v):
    return v.z < CHIN_CUT


def _bisect(bm, planes):
    for co, no in planes:
        geom = list(bm.verts) + list(bm.edges) + list(bm.faces)
        bmesh.ops.bisect_plane(bm, geom=geom, dist=EPS,
                               plane_co=co, plane_no=no,
                               clear_inner=False, clear_outer=False)
    bm.faces.ensure_lookup_table()


def build_helmet():
    old = bpy.data.objects.get(NAME)
    if old:
        d = old.data
        bpy.data.objects.remove(old)
        bpy.data.meshes.remove(d)

    mats = ensure_materials()

    # ------------------------------------------------------------- carcasa
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=72, v_segments=40, radius=1.0)
    for v in bm.verts:
        v.co = _shape(v.co)
    _bisect(bm, [((0.0, VISOR_Y, 0.0), (0.0, 1.0, 0.0)),
                 ((0.0, 0.0, VISOR_Z0), (0.0, 0.0, 1.0)),
                 ((0.0, 0.0, VISOR_Z1), (0.0, 0.0, 1.0)),
                 ((0.0, 0.0, CHIN_CUT), (0.0, 0.0, 1.0))])

    dead = [f for f in bm.faces
            if _in_visor(f.calc_center_median()) or _below_jaw(f.calc_center_median())]
    bmesh.ops.delete(bm, geom=dead, context='FACES')

    # Trim por distancia al borde de la abertura, no por adyacencia: tras el
    # bisect el anillo de borde tiene caras de tamanos muy distintos y la
    # adyacencia producia un marco de ancho irregular (aspecto escalonado).
    boundary = [v.co.copy() for e in bm.edges if e.is_boundary for v in e.verts]
    for f in bm.faces:
        c = f.calc_center_median()
        near = any((c - b).length < TRIM_WIDTH for b in boundary)
        f.material_index = TRIM if near else SHELL
        f.smooth = True

    me = bpy.data.meshes.new(NAME + '_Mesh')
    bm.to_mesh(me)
    bm.free()

    ob = bpy.data.objects.new(NAME, me)
    bpy.data.collections[COLLECTION].objects.link(ob)
    for m in mats:
        me.materials.append(m)

    # ---------------------------------------------------------------- visor
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=72, v_segments=40, radius=1.0)
    for v in bm.verts:
        v.co = _shape(v.co, 0.955)   # hundido: la carcasa enmarca el visor
    # margen: el visor se solapa por detras del borde, si no queda una rendija
    M = 0.016
    _bisect(bm, [((0.0, VISOR_Y + M, 0.0), (0.0, 1.0, 0.0)),
                 ((0.0, 0.0, VISOR_Z0 - M), (0.0, 0.0, 1.0)),
                 ((0.0, 0.0, VISOR_Z1 + M), (0.0, 0.0, 1.0)),
                 ((0.0, 0.0, CHIN_CUT), (0.0, 0.0, 1.0))])
    keep = [f for f in bm.faces
            if _in_visor(f.calc_center_median(), M) and not _below_jaw(f.calc_center_median())]
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f not in keep], context='FACES')
    for f in bm.faces:
        f.material_index = GLASS
        f.smooth = True
    visor = bpy.data.meshes.new('tmp_visor')
    bm.to_mesh(visor)
    bm.free()

    # --------------------------------------------------- luces de sien
    strips = []
    for sgn in (1.0, -1.0):
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1.0)
        for v in bm.verts:
            v.co = Vector((v.co.x * 0.008, v.co.y * 0.090, v.co.z * 0.008))
            v.co += Vector((sgn * 0.1055, 0.006, 1.756))
        for f in bm.faces:
            f.material_index = EMISSION
        m = bpy.data.meshes.new('tmp_strip')
        bm.to_mesh(m)
        bm.free()
        strips.append(m)

    # ---------------------------------------------- union de las partes
    for extra, offset in [(visor, GLASS)] + [(s, EMISSION) for s in strips]:
        bm = bmesh.new()
        bm.from_mesh(me)
        tmp = bmesh.new()
        tmp.from_mesh(extra)
        base_index = offset
        verts = [bm.verts.new(v.co) for v in tmp.verts]
        bm.verts.index_update()
        tmp.faces.ensure_lookup_table()
        for f in tmp.faces:
            try:
                nf = bm.faces.new([verts[v.index] for v in f.verts])
            except ValueError:
                continue
            nf.material_index = base_index
            nf.smooth = (base_index == GLASS)
        tmp.free()
        bm.to_mesh(me)
        bm.free()
        bpy.data.meshes.remove(extra)

    sol = ob.modifiers.new('SOL_Shell', 'SOLIDIFY')
    sol.thickness = SHELL_THICKNESS
    sol.offset = 1.0
    sol.use_rim = True
    sol.material_offset_rim = 0
    return ob


def bind_to_head(ob, rig=None):
    """Pondera el casco al hueso `head` con peso 1 y lo deja como skinned mesh."""
    rig = rig or bpy.data.objects['SKEL_Astraeon_Player']
    vg = ob.vertex_groups.get('head') or ob.vertex_groups.new(name='head')
    vg.add(list(range(len(ob.data.vertices))), 1.0, 'REPLACE')
    ob.parent = rig
    m = ob.modifiers.get('ARM_Skin') or ob.modifiers.new('ARM_Skin', 'ARMATURE')
    m.object = rig
    return ob


if __name__ == '__main__':
    bind_to_head(build_helmet())
