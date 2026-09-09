"""Editable, locally authored helmet and expedition equipment in metre scale.

Continuous lofts establish the shell, chamfers, inset panels and attachment
surfaces. Source body and previous helmet are retained for comparison.
"""
import math
import bpy
import bmesh
from mathutils import Vector, Matrix

RIG = 'SKEL_Astraeon_Player'
COLLECTION = 'ASTRAEON_EQUIPMENT'
PALETTE = {
    'Ceramic': ((.68, .665, .61, 1), .4, .05, 0),
    'Graphite': ((.035, .045, .055, 1), .48, .35, 0),
    'Bronze': ((.30, .16, .065, 1), .32, .8, 0),
    'Visor': ((.008, .018, .025, 1), .2, .3, 0),
    'Display': ((.012, .25, .30, 1), .3, 0, 2),
}


def material(role):
    name = 'MAT_Player_Equipment_' + role
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    node = next(n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    color, roughness, metallic, emission = PALETTE[role]
    values = {'Base Color': color, 'Roughness': roughness, 'Metallic': metallic,
              'Emission Color': color, 'Emission Strength': emission}
    for socket in node.inputs:
        if socket.identifier in values:
            socket.default_value = values[socket.identifier]
    mat.diffuse_color = color
    return mat


class Builder:
    def __init__(self):
        self.vertices, self.faces, self.slots = [], [], []
        self.roles = list(PALETTE)

    def add_face(self, indices, role):
        self.faces.append(tuple(indices))
        self.slots.append(self.roles.index(role))

    def loft(self, rings, role='Graphite', cap_start=True, cap_end=True, ring_roles=None):
        offset, n = len(self.vertices), len(rings[0])
        self.vertices.extend(tuple(p) for ring in rings for p in ring)
        if cap_start:
            self.add_face([offset+i for i in reversed(range(n))], role)
        for r in range(len(rings)-1):
            for i in range(n):
                selected_role = ring_roles(r, i) if ring_roles else role
                self.add_face([offset+r*n+i, offset+r*n+(i+1)%n,
                               offset+(r+1)*n+(i+1)%n, offset+(r+1)*n+i], selected_role)
        if cap_end:
            self.add_face([offset+(len(rings)-1)*n+i for i in range(n)], role)

    def panel(self, center, width, height, depth, role, transform=None):
        """Chamfered continuous panel with a stepped shoulder and recessed face."""
        x, y, z = center
        rings = []
        profile = [(-.5,-.38),(-.38,-.5),(.38,-.5),(.5,-.38),
                   (.5,.38),(.38,.5),(-.38,.5),(-.5,.38)]
        for d, scale in [(0,.90),(depth*.18,1),(depth*.82,1),(depth,.88)]:
            ring = [Vector((x+a*width*scale, y+d, z+b*height*scale)) for a,b in profile]
            rings.append([transform@p if transform else p for p in ring])
        self.loft(rings, role)

    def build(self, name, bone):
        assert not bpy.data.objects.get(name), 'Preserve existing equipment: ' + name
        mesh = bpy.data.meshes.new(name + '_Mesh')
        mesh.from_pydata(self.vertices, [], self.faces)
        mesh.update()
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        bm.to_mesh(mesh)
        bm.free()
        for role in self.roles:
            mesh.materials.append(material(role))
        for p, slot in zip(mesh.polygons, self.slots):
            p.material_index = slot
        obj = bpy.data.objects.new(name, mesh)
        coll = bpy.data.collections.get(COLLECTION)
        if coll is None:
            coll = bpy.data.collections.new(COLLECTION)
            bpy.context.scene.collection.children.link(coll)
        coll.objects.link(obj)
        rig = bpy.data.objects[RIG]
        rig.data.bones[bone].use_deform = True
        obj.parent = rig
        group = obj.vertex_groups.new(name=bone)
        group.add(list(range(len(mesh.vertices))), 1.0, 'REPLACE')
        modifier = obj.modifiers.new('ARM_Equipment', 'ARMATURE')
        modifier.object = rig
        obj['attachment_bone'] = bone
        obj['construction'] = 'continuous lofts; local original geometry'
        return obj


def helmet(detail=True):
    old = bpy.data.objects.get('SK_Astraeon_Helmet')
    if old:
        old.name = 'ARCHIVE_Helmet_DesignShell' if old.get('construction') else 'ARCHIVE_Helmet_Blockout'
        old.hide_render = True
        old.hide_set(True)
    b = Builder()
    # Z, semiwidth, rear depth, front depth. Explicit face rows avoid the
    # old sphere-bisect's staircase edges on the visor opening.
    stations = [(1.580,.086,.090,.142), (1.617,.095,.108,.155),
                (1.645,.111,.126,.162), (1.657,.119,.134,.166),
                (1.663,.123,.138,.166), (1.777,.125,.143,.164),
                (1.785,.124,.143,.164), (1.798,.120,.140,.159),
                (1.826,.102,.120,.130), (1.855,.058,.068,.071),
                (1.862,.024,.028,.028)]
    segments = 32
    rings=[]
    for z, width, rear, front in stations:
        rings.append([(math.sin(a)*width, -.006-math.cos(a)*(front if math.cos(a)>0 else rear), z)
                      for a in [2*math.pi*i/segments for i in range(segments)]])
    def roles(row, i):
        front_arc = min(i, segments-1-i) <= 6
        if row == 4 and front_arc:
            return 'Visor'
        if row in (3,5) and front_arc:
            return 'Bronze'
        if row == 4 and min(i, segments-1-i) == 7:
            return 'Graphite'
        if row == 0 or (row == 2 and front_arc):
            return 'Graphite'
        return 'Ceramic'
    b.loft(rings, 'Ceramic', cap_start=False, ring_roles=roles)
    # Rear service recess, protected under a stepped graphite inset panel.
    if detail:
        b.panel((0,.132,1.714), .105,.083,.006,'Graphite')
        for x in (-.033,-.011,.011,.033):
            b.panel((x,.139,1.714), .009,.043,.002,'Bronze')
    obj = b.build('SK_Astraeon_Helmet','socket_helmet')
    shell = obj.modifiers.new('Shell_thickness_3mm','SOLIDIFY')
    shell.thickness=.003
    shell.offset=-1
    # Skinning runs last so the thickness follows the rest-space shell.
    bpy.context.view_layer.objects.active=obj
    bpy.ops.object.modifier_move_up(modifier=shell.name)
    return obj


# Espalda real del traje medida sobre SK_Astraeon_Player en la franja central
# (|x| < 0.10): y maxima del torso por altura. La columna describe un hueco
# lumbar en z ~ 1.13 y el punto mas prominente en los omoplatos, z ~ 1.40.
# El frente de la mochila se apoya sobre esta curva mas una holgura de 4 mm.
BACK_CURVE=[(1.06,.0974),(1.10,.0768),(1.14,.0726),(1.18,.0878),(1.22,.1101),
            (1.26,.1528),(1.30,.1707),(1.34,.1842),(1.38,.1899),(1.42,.1904),
            (1.46,.1874),(1.50,.1742),(1.52,.1678)]
BACK_CLEARANCE=.004


def back_at(z):
    """Interpola la espalda medida; extrapola plano fuera del rango medido."""
    zs=[p[0] for p in BACK_CURVE]; ys=[p[1] for p in BACK_CURVE]
    if z<=zs[0]: return ys[0]
    if z>=zs[-1]: return ys[-1]
    for i in range(len(zs)-1):
        if zs[i]<=z<=zs[i+1]:
            t=(z-zs[i])/(zs[i+1]-zs[i])
            return ys[i]+(ys[i+1]-ys[i])*t
    return ys[-1]


def front_at(z):
    return back_at(z)+BACK_CLEARANCE


# Cara trasera de la carcasa: arco suave propio de la pieza, independiente del
# cuerpo. Junto con el frente define el espesor variable de cada anillo.
SHELL_REAR=[(1.09,.245),(1.14,.258),(1.20,.272),(1.26,.280),
            (1.32,.282),(1.40,.278),(1.475,.265),(1.505,.252)]
SHELL_WIDTH={1.09:.112,1.14:.130,1.20:.145,1.26:.150,
             1.32:.150,1.40:.150,1.475:.135,1.505:.100}


def rear_at(z):
    zs=[p[0] for p in SHELL_REAR]; ys=[p[1] for p in SHELL_REAR]
    if z<=zs[0]: return ys[0]
    if z>=zs[-1]: return ys[-1]
    for i in range(len(zs)-1):
        if zs[i]<=z<=zs[i+1]:
            t=(z-zs[i])/(zs[i+1]-zs[i])
            return ys[i]+(ys[i+1]-ys[i])*t
    return ys[-1]


def backpack(detail=True):
    b=Builder()
    # Broad shoulder, narrow lower back, with 12 mm chamfers along the shell.
    # El frente sigue la espalda medida; el trasero sigue su propio arco, de modo
    # que la pieza apoya en toda su altura en vez de flotar sobre la cintura.
    rings=[]
    profile=[(-1,-.65),(-.82,-1),(.82,-1),(1,-.65),(1,.65),(.82,1),(-.82,1),(-1,.65)]
    for z,_ in SHELL_REAR:
        front,rear=front_at(z),rear_at(z)
        cy,dep,w=(front+rear)/2,(rear-front)/2,SHELL_WIDTH[z]
        rings.append([(x*w,cy+y*dep,z) for x,y in profile])
    b.loft(rings,'Graphite')
    if detail:
        # Apoyos en los dos puntos de contacto reales: lumbar bajo y omoplatos.
        for z,width,height in ((1.19,.16,.085),(1.43,.19,.060)):
            b.panel((0,front_at(z)-.003,z),width,height,.030,'Graphite')
        # `panel` mide desde su cara frontal, no desde el centro: los offsets son
        # los mismos del diseno original, ahora relativos a la carcasa curvada.
        b.panel((0,rear_at(1.325),1.325),.235,.23,.013,'Ceramic')
        b.panel((0,rear_at(1.325)+.014,1.325),.157,.135,.003,'Graphite')
        for z in (1.285,1.31,1.335,1.36):
            b.panel((0,rear_at(z)+.018,z),.116,.008,.0015,'Bronze')
        b.panel((0,rear_at(1.455)+.001,1.455),.085,.021,.006,'Display')
        for x in (-.083,.083):
            b.panel((x,rear_at(1.15)-.012,1.15),.045,.063,.010,'Bronze')
    obj=b.build('SK_Astraeon_Backpack','socket_backpack')
    obj['attachment_socket']='socket_backpack'
    return obj


def wrist(detail=True):
    b=Builder()
    rig=bpy.data.objects[RIG]
    bone=rig.data.bones['lowerarm_l']
    axis=(bone.tail_local-bone.head_local).normalized()
    up=Vector((0,0,1))
    lateral=up.cross(axis).normalized()
    up=axis.cross(lateral).normalized()
    center=bone.head_local.lerp(bone.tail_local,.71)+up*.047
    # panel depth axis is outward/up; screen length aligns with the forearm.
    mat=Matrix(((axis.x,up.x,lateral.x,center.x),
                (axis.y,up.y,lateral.y,center.y),
                (axis.z,up.z,lateral.z,center.z),(0,0,0,1)))
    b.panel((0,0,0),.108,.075,.017,'Graphite',mat)
    b.panel((0,.017,0),.091,.061,.003,'Bronze',mat)
    b.panel((0,.020,0),.077,.048,.002,'Visor',mat)
    # Native geometry readouts, individually editable in the base mesh.
    if detail:
        for z,length in [(.012,.048),(0,.059),(-.012,.033)]:
            b.panel((-.006,.022,z),length,.003,.0005,'Display',mat)
        for x in (-.027,0,.027):
            b.panel((x,.014,-.031),.014,.006,.003,'Ceramic',mat)
    obj=b.build('SK_Astraeon_WristComputer','lowerarm_l')
    obj['attachment_socket']='lowerarm_l'
    return obj


def build(detail=True):
    for name in ('SK_Astraeon_Backpack', 'SK_Astraeon_WristComputer'):
        obj = bpy.data.objects.get(name)
        if obj:
            assert obj.get('construction'), 'Refuse replacing unowned equipment'
            obj.name = 'ARCHIVE_' + name.removeprefix('SK_Astraeon_') + '_DesignShell'
            obj.hide_render = True
            obj.hide_set(True)
    return [helmet(detail), backpack(detail), wrist(detail)]
