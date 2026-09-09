"""Original Q1 quadruped: Umbra Grazer, a palette/plate variant, and seven state clips.

Metres, +Z up, -Y forward. Self-contained on purpose: the humanoid generator's sha256 is an
input to an already-approved validation report, so it stays frozen instead of being
refactored into shared helpers. Run in a fresh background Blender process.
"""
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
import bmesh
from mathutils import Vector, Quaternion

TOOL_ROOT = Path(__file__).resolve().parents[1]
ROOT = TOOL_ROOT.parents[1]
sys.path.insert(0, str(TOOL_ROOT))
from common.scene import reset_scene, material
from validators.creature import validate_source, validate_actions, negative_controls

OUT = ROOT / 'ContentPipeline/Generated/CreatureBlockout'
REPORT = ROOT / 'ContentPipeline/reports/creature_blockout_validation.json'
CONFIG_PATH = TOOL_ROOT / 'configs/creature_blockout.json'
CFG = json.loads(CONFIG_PATH.read_text(encoding='utf-8'))
RIG_NAME = 'SKEL_Quadruped_A'
TAIL = ['tail_01', 'tail_02', 'tail_03', 'tail_04']
SIDES = [('l', 1), ('r', -1)]
BONES = {}
PARTS = []


def bone(name, head, tail, parent=None):
    BONES[name] = (Vector(head), Vector(tail), parent)
    return name


def make_rig():
    """Torso, neck/head/jaw, four three-joint limb chains, four-segment tail, sockets."""
    BONES.clear()
    bone('root', (0, 0, 0), (0, 0, .14))
    bone('pelvis', (0, .30, .50), (0, .14, .52), 'root')
    bone('spine_01', (0, .14, .52), (0, -.02, .545), 'pelvis')
    bone('spine_02', (0, -.02, .545), (0, -.18, .55), 'spine_01')
    bone('spine_03', (0, -.18, .55), (0, -.34, .535), 'spine_02')
    bone('neck_01', (0, -.34, .535), (0, -.46, .565), 'spine_03')
    bone('neck_02', (0, -.46, .565), (0, -.56, .585), 'neck_01')
    bone('head', (0, -.56, .585), (0, -.70, .565), 'neck_02')
    bone('jaw', (0, -.615, .545), (0, -.725, .505), 'head')

    parent = 'pelvis'
    for index, (head, tail) in enumerate([((0, .30, .50), (0, .41, .475)),
                                          ((0, .41, .475), (0, .51, .445)),
                                          ((0, .51, .445), (0, .60, .41)),
                                          ((0, .60, .41), (0, .68, .375))], 1):
        parent = bone(f'tail_{index:02}', head, tail, parent)

    for side, sign in SIDES:
        x = lambda value: value * sign
        bone('shoulder_' + side, (x(.075), -.28, .525), (x(.175), -.28, .495), 'spine_03')
        bone('upperleg_f_' + side, (x(.175), -.28, .495), (x(.185), -.225, .305), 'shoulder_' + side)
        bone('lowerleg_f_' + side, (x(.185), -.225, .305), (x(.19), -.285, .125), 'upperleg_f_' + side)
        bone('foot_f_' + side, (x(.19), -.285, .125), (x(.19), -.345, .042), 'lowerleg_f_' + side)
        bone('toe_f_' + side, (x(.19), -.345, .042), (x(.19), -.415, .032), 'foot_f_' + side)
        bone('hip_' + side, (x(.08), .265, .505), (x(.185), .265, .475), 'pelvis')
        bone('upperleg_b_' + side, (x(.185), .265, .475), (x(.195), .335, .295), 'hip_' + side)
        bone('lowerleg_b_' + side, (x(.195), .335, .295), (x(.20), .235, .125), 'upperleg_b_' + side)
        bone('foot_b_' + side, (x(.20), .235, .125), (x(.20), .165, .042), 'lowerleg_b_' + side)
        bone('toe_b_' + side, (x(.20), .165, .042), (x(.20), .098, .032), 'foot_b_' + side)

    bone('socket_plate_back', (0, -.10, .70), (0, -.10, .78), 'spine_02')
    bone('socket_horn_l', (.055, -.645, .655), (.055, -.645, .745), 'head')
    bone('socket_horn_r', (-.055, -.645, .655), (-.055, -.645, .745), 'head')
    bone('socket_sensor', (0, -.70, .60), (0, -.775, .60), 'head')

    data = bpy.data.armatures.new(RIG_NAME)
    rig = bpy.data.objects.new(RIG_NAME, data)
    bpy.context.collection.objects.link(rig)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    for name, (head, tail, parent) in BONES.items():
        edit_bone = data.edit_bones.new(name)
        edit_bone.head, edit_bone.tail = head, tail
        edit_bone.align_roll(Vector((0, 0, 1)))
        if parent:
            edit_bone.parent = data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    rig.show_in_front = True
    rig.data.display_type = 'STICK'
    rig['generator_version'] = CFG['generator_version']
    rig['forward_axis'] = '-Y'
    rig['quality'] = 'Q1 prototype; no retarget compatibility claim'
    return rig


def add_part(vertices, faces, weights, mat, plated=False):
    # bpy's UV sphere operator can enumerate identical polygons in different order.
    # Canonicalize polygon start without changing winding or topology.
    canonical = []
    for face in faces:
        indices = tuple(face)
        start = indices.index(min(indices))
        canonical.append(indices[start:] + indices[:start])
    PARTS.append((vertices, sorted(canonical), weights, mat, plated))


def tube(chain, radii, mat='hide', sides=12, plated=False):
    """Closed ring surface with blended weights across every joint."""
    vertices, faces, weights = [], [], []
    for index, name in enumerate(chain):
        head, tail, _ = BONES[name]
        direction = (tail - head).normalized()
        depth = Vector((0, 1, 0))
        if abs(depth.dot(direction)) > .95:
            depth = Vector((1, 0, 0))
        across = direction.cross(depth).normalized()
        depth = across.cross(direction).normalized()
        for t in ([0, .22, .78] if index < len(chain) - 1 else [0, .22, .78, 1]):
            center = head.lerp(tail, t)
            width, thick = radii[index]
            taper = .82 if index == len(chain) - 1 and t == 1 else 1
            weight = {name: 1.0}
            if t == 0 and index > 0:
                weight = {chain[index - 1]: .5, name: .5}
            elif t == .22 and index > 0:
                weight = {chain[index - 1]: .15, name: .85}
            elif t == .78 and index < len(chain) - 1:
                weight = {name: .85, chain[index + 1]: .15}
            for j in range(sides):
                angle = math.tau * j / sides
                vertices.append(center + taper * (across * width * math.cos(angle)
                                                  + depth * thick * math.sin(angle)))
                weights.append(weight)
    rings = len(vertices) // sides
    for ring in range(rings - 1):
        for j in range(sides):
            a, b = ring * sides + j, ring * sides + (j + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    for ring, reverse in [(0, True), (rings - 1, False)]:
        ids = list(range(ring * sides, (ring + 1) * sides))
        center = sum((vertices[k] for k in ids), Vector()) / sides
        centre_index = len(vertices)
        vertices.append(center)
        weights.append(weights[ids[0]])
        for j in range(sides):
            a, b = ids[j], ids[(j + 1) % sides]
            faces.append((centre_index, b, a) if reverse else (centre_index, a, b))
    add_part(vertices, faces, weights, mat, plated)


def ellipsoid(center, radius, name, mat, segments=16, rings=10, plated=False):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=center)
    obj = bpy.context.object
    vertices = [Vector(center) + Vector(tuple(v.co[i] * radius[i] for i in range(3)))
                for v in obj.data.vertices]
    add_part(vertices, [tuple(p.vertices) for p in obj.data.polygons],
             [{name: 1.0}] * len(vertices), mat, plated)
    bpy.data.objects.remove(obj, do_unlink=True)


def box(center, scale, name, mat, bevel=.012, plated=False):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj = bpy.context.object
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    modifier = obj.modifiers.new('Machined edges', 'BEVEL')
    modifier.width, modifier.segments = min(bevel, min(scale) / 3), 2
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    add_part([obj.matrix_world @ v.co for v in obj.data.vertices],
             [tuple(p.vertices) for p in obj.data.polygons],
             [{name: 1.0}] * len(obj.data.vertices), mat, plated)
    bpy.data.objects.remove(obj, do_unlink=True)


def make_parts():
    """Low, wide grazer: heavy barrel, short neck, head carried below the shoulders."""
    PARTS.clear()
    tube(['pelvis', 'spine_01', 'spine_02', 'spine_03'],
         [(.150, .160), (.162, .172), (.165, .175), (.150, .162)])
    ellipsoid((0, .30, .50), (.155, .145, .145), 'pelvis', 'hide')
    ellipsoid((0, -.28, .535), (.175, .140, .160), 'spine_03', 'hide')
    tube(['neck_01', 'neck_02'], [(.078, .082), (.070, .074)])
    ellipsoid((0, -.625, .578), (.098, .098, .092), 'head', 'plate', segments=20, rings=14)
    box((0, -.668, .523), (.098, .112, .052), 'jaw', 'plate', bevel=.010)
    # Los ojos son el material de piel sobre la carcasa clara: leen oscuros sin gastar
    # una tercera ranura de material.
    for _, sign in SIDES:
        ellipsoid((sign * .072, -.672, .598), (.023, .021, .021), 'head', 'hide', segments=8, rings=6)
    tube(TAIL, [(.055, .055), (.042, .042), (.032, .032), (.022, .022)], sides=10)

    for side, sign in SIDES:
        ellipsoid((sign * .155, -.28, .48), (.075, .100, .105), 'shoulder_' + side, 'hide')
        ellipsoid((sign * .160, .265, .46), (.080, .115, .115), 'hip_' + side, 'hide')
        tube(['upperleg_f_' + side, 'lowerleg_f_' + side, 'foot_f_' + side],
             [(.055, .060), (.042, .045), (.032, .034)])
        tube(['upperleg_b_' + side, 'lowerleg_b_' + side, 'foot_b_' + side],
             [(.062, .068), (.045, .048), (.034, .036)])
        box((sign * .19, -.315, .060), (.085, .105, .090), 'foot_f_' + side, 'plate')
        box((sign * .19, -.385, .026), (.088, .100, .052), 'toe_f_' + side, 'plate', bevel=.008)
        box((sign * .20, .200, .060), (.085, .105, .090), 'foot_b_' + side, 'plate')
        box((sign * .20, .130, .026), (.088, .100, .052), 'toe_b_' + side, 'plate', bevel=.008)

    # Sólo la variante: placas dorsales ligeras sobre el mismo esqueleto.
    box((0, -.100, .695), (.170, .300, .052), 'spine_02', 'plate', bevel=.014, plated=True)
    box((0, .100, .680), (.155, .260, .050), 'spine_01', 'plate', bevel=.014, plated=True)
    box((0, -.450, .615), (.100, .160, .042), 'neck_01', 'plate', bevel=.010, plated=True)


def make_mesh(spec, rig):
    vertices, faces, weights, materials = [], [], [], []
    for part_vertices, part_faces, part_weights, mat, plated in PARTS:
        if plated and not spec['plated']:
            continue
        offset = len(vertices)
        vertices.extend(part_vertices)
        faces.extend(tuple(offset + k for k in face) for face in part_faces)
        weights.extend(part_weights)
        materials.extend([mat] * len(part_faces))
    mesh = bpy.data.meshes.new(spec['id'] + '_Geometry')
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    palette = list(spec['palette'])
    suffix = '' if spec['variant'] == 'base' else '_' + spec['variant'].capitalize()
    for key in palette:
        mesh.materials.append(material('M_Creature_' + key.capitalize() + suffix, spec['palette'][key]))
    for polygon, key in zip(mesh.polygons, materials):
        polygon.material_index = palette.index(key)
        polygon.use_smooth = True
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(spec['id'], mesh)
    bpy.context.collection.objects.link(obj)
    for key in BONES:
        group = obj.vertex_groups.new(name=key)
        for index, weight in enumerate(weights):
            if key in weight:
                group.add([index], weight[key], 'REPLACE')
    obj.parent = rig
    modifier = obj.modifiers.new('Shared quadruped deformation', 'ARMATURE')
    modifier.object = rig
    obj['quality'] = 'Q1; original procedural prototype'
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(island_margin=.015)
    bpy.ops.object.mode_set(mode='OBJECT')
    return obj


def clear_pose(rig):
    for pose_bone in rig.pose.bones:
        pose_bone.rotation_mode = 'QUATERNION'
        pose_bone.rotation_quaternion = (1, 0, 0, 0)
        pose_bone.location = (0, 0, 0)
        pose_bone.scale = (1, 1, 1)


def rotate(rig, name, axis, degrees):
    """Compose a rotation about an armature-space axis onto whatever the bone already has."""
    pose_bone = rig.pose.bones[name]
    rest = pose_bone.bone.matrix_local.to_quaternion()
    delta = rest.inverted() @ Quaternion(Vector(axis), math.radians(degrees)) @ rest
    pose_bone.rotation_quaternion = pose_bone.rotation_quaternion @ delta


def translate_world(rig, name, offset):
    pose_bone = rig.pose.bones[name]
    pose_bone.location = pose_bone.bone.matrix_local.to_3x3().inverted() @ Vector(offset)


def pose(rig, kind, t):
    """Set every rotation for one frame and return the intended lift off the ground."""
    clear_pose(rig)
    cycle = math.sin(math.tau * t)
    envelope = math.sin(math.pi * t) ** 2
    lift = Vector((0, 0, 0))

    if kind == 'Graze':
        rotate(rig, 'neck_01', (1, 0, 0), 40)
        rotate(rig, 'neck_02', (1, 0, 0), 32)
        rotate(rig, 'head', (1, 0, 0), 14)
        # Masticar corre cuatro veces más rápido que la respiración.
        rotate(rig, 'jaw', (1, 0, 0), -9 - 8 * math.sin(math.tau * 4 * t))
        rotate(rig, 'spine_02', (1, 0, 0), 1.4 * cycle)
        for index, name in enumerate(TAIL, 1):
            rotate(rig, name, (0, 0, 1), 4 * index * cycle)

    elif kind == 'Walk':
        # Trote diagonal: delantera de un lado con trasera del otro.
        for side, sign in SIDES:
            swing = cycle * sign
            rotate(rig, 'upperleg_f_' + side, (1, 0, 0), -26 * swing)
            rotate(rig, 'lowerleg_f_' + side, (1, 0, 0), 34 * max(0.0, swing))
            rotate(rig, 'foot_f_' + side, (1, 0, 0), -16 * max(0.0, swing))
            rotate(rig, 'upperleg_b_' + side, (1, 0, 0), 26 * swing)
            rotate(rig, 'lowerleg_b_' + side, (1, 0, 0), -30 * max(0.0, -swing))
            rotate(rig, 'foot_b_' + side, (1, 0, 0), 14 * max(0.0, -swing))
        rotate(rig, 'spine_02', (1, 0, 0), 2.5 * math.sin(math.tau * 2 * t))
        rotate(rig, 'neck_01', (1, 0, 0), 16)
        rotate(rig, 'head', (1, 0, 0), -10)
        for index, name in enumerate(TAIL, 1):
            rotate(rig, name, (0, 0, 1), 3 * index * cycle)
        lift = Vector((0, 0, .012 * (1 - math.cos(2 * math.tau * t))))

    elif kind == 'Alert':
        rotate(rig, 'neck_01', (1, 0, 0), -28)
        rotate(rig, 'neck_02', (1, 0, 0), -22)
        rotate(rig, 'head', (1, 0, 0), 10)
        # Barrido de cabeza: busca de dónde vino el ruido en vez de quedarse rígida.
        rotate(rig, 'head', (0, 0, 1), 11 * cycle)
        rotate(rig, 'jaw', (1, 0, 0), -4)
        rotate(rig, 'spine_02', (1, 0, 0), 1.0 * cycle)
        rotate(rig, 'tail_01', (1, 0, 0), -20)
        for index, name in enumerate(TAIL, 1):
            rotate(rig, name, (0, 0, 1), 2 * index * cycle)
        for side, _ in SIDES:
            rotate(rig, 'upperleg_b_' + side, (1, 0, 0), -7)
            rotate(rig, 'lowerleg_b_' + side, (1, 0, 0), 11)

    elif kind == 'Threaten':
        rotate(rig, 'neck_01', (1, 0, 0), 22)
        rotate(rig, 'neck_02', (1, 0, 0), 14)
        rotate(rig, 'head', (1, 0, 0), -20)
        rotate(rig, 'jaw', (1, 0, 0), -20 - 10 * abs(cycle))
        rotate(rig, 'spine_03', (1, 0, 0), -7)
        rotate(rig, 'spine_02', (1, 0, 0), 2.0 * math.sin(math.tau * 2 * t))
        # La cola late al doble del ciclo: es el aviso de que va a embestir.
        for index, name in enumerate(TAIL, 1):
            rotate(rig, name, (0, 0, 1), 5 * index * math.sin(math.tau * 2 * t))
        for side, _ in SIDES:
            rotate(rig, 'upperleg_f_' + side, (1, 0, 0), -9)
            rotate(rig, 'lowerleg_f_' + side, (1, 0, 0), 12)
            rotate(rig, 'upperleg_b_' + side, (1, 0, 0), 11)
            rotate(rig, 'lowerleg_b_' + side, (1, 0, 0), -14)

    elif kind == 'Flee':
        # Galope: las dos delanteras juntas y las dos traseras juntas, desfasadas.
        rear = math.sin(math.tau * t + math.pi * .55)
        for side, _ in SIDES:
            rotate(rig, 'upperleg_f_' + side, (1, 0, 0), -42 * cycle)
            rotate(rig, 'lowerleg_f_' + side, (1, 0, 0), 54 * max(0.0, cycle))
            rotate(rig, 'foot_f_' + side, (1, 0, 0), -22 * max(0.0, cycle))
            rotate(rig, 'upperleg_b_' + side, (1, 0, 0), 40 * rear)
            rotate(rig, 'lowerleg_b_' + side, (1, 0, 0), -46 * max(0.0, -rear))
            rotate(rig, 'foot_b_' + side, (1, 0, 0), 20 * max(0.0, -rear))
        rotate(rig, 'spine_02', (1, 0, 0), 13 * math.sin(math.tau * t + math.pi * .25))
        rotate(rig, 'spine_01', (1, 0, 0), 8 * math.sin(math.tau * t + math.pi * .25))
        rotate(rig, 'neck_01', (1, 0, 0), -8)
        rotate(rig, 'head', (1, 0, 0), 6)
        for index, name in enumerate(TAIL, 1):
            rotate(rig, name, (1, 0, 0), -9 - 2 * index)
        lift = Vector((0, 0, .022 * (1 - math.cos(2 * math.tau * t))))

    elif kind == 'Hit':
        rotate(rig, 'neck_01', (1, 0, 0), -34 * envelope)
        rotate(rig, 'neck_02', (1, 0, 0), -20 * envelope)
        rotate(rig, 'head', (1, 0, 0), -12 * envelope)
        rotate(rig, 'jaw', (1, 0, 0), -26 * envelope)
        rotate(rig, 'spine_02', (1, 0, 0), -12 * envelope)
        rotate(rig, 'spine_01', (1, 0, 0), -7 * envelope)
        for side, _ in SIDES:
            rotate(rig, 'upperleg_f_' + side, (1, 0, 0), 10 * envelope)
            rotate(rig, 'lowerleg_f_' + side, (1, 0, 0), -16 * envelope)
            rotate(rig, 'upperleg_b_' + side, (1, 0, 0), -9 * envelope)
            rotate(rig, 'lowerleg_b_' + side, (1, 0, 0), 15 * envelope)
        for index, name in enumerate(TAIL, 1):
            rotate(rig, name, (0, 0, 1), 6 * index * envelope)

    elif kind == 'Death':
        progress = t * t * (3 - 2 * t)
        # Las patas se abren hasta quedar horizontales, a la altura del propio torso. Ni
        # plegadas hacia atrás (quedan apoyadas y lo sostienen) ni abiertas de más (la
        # caña hereda el giro del muslo y la pata termina por encima del lomo): lo que
        # baja al animal es que el conjunto entero quepa en el alto de un cuerpo tumbado.
        for side, sign in SIDES:
            rotate(rig, 'upperleg_f_' + side, (0, 1, 0), -86 * sign * progress)
            rotate(rig, 'lowerleg_f_' + side, (0, 1, 0), 12 * sign * progress)
            rotate(rig, 'foot_f_' + side, (1, 0, 0), 18 * progress)
            rotate(rig, 'upperleg_b_' + side, (0, 1, 0), -84 * sign * progress)
            rotate(rig, 'lowerleg_b_' + side, (0, 1, 0), 14 * sign * progress)
            rotate(rig, 'foot_b_' + side, (1, 0, 0), -16 * progress)
        rotate(rig, 'pelvis', (0, 1, 0), 8 * progress)
        rotate(rig, 'spine_01', (1, 0, 0), 10 * progress)
        # El cuello baja hasta apoyar la cabeza al nivel del vientre. Más que eso y el animal
        # queda colgado de su propia cabeza, que pasa a ser el punto de apoyo del cuerpo.
        rotate(rig, 'neck_01', (1, 0, 0), 20 * progress)
        rotate(rig, 'neck_02', (1, 0, 0), 14 * progress)
        rotate(rig, 'head', (1, 0, 0), 8 * progress)
        rotate(rig, 'jaw', (1, 0, 0), -20 * progress)
        for name in TAIL:
            rotate(rig, name, (1, 0, 0), 12 * progress)

    return lift


def lowest_z(obj):
    evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    try:
        return min(v.co.z for v in mesh.vertices)
    finally:
        evaluated.to_mesh_clear()


def apply_pose(rig, body, kind, t):
    """Pose, then settle on the floor and add the clip's intended lift.

    Grounding every frame is what keeps a blockout honest: no clip can float or sink,
    and any vertical motion has to be declared as lift instead of appearing by accident.
    """
    lift = pose(rig, kind, t)
    translate_world(rig, 'pelvis', Vector((0, 0, 0)))
    bpy.context.view_layer.update()
    translate_world(rig, 'pelvis', Vector((0, 0, -lowest_z(body))) + lift)
    bpy.context.view_layer.update()


def make_actions(rig, body):
    rig.animation_data_create()
    actions = {}
    scene = bpy.context.scene
    scene.render.fps = CFG['fps']
    for kind, spec in CFG['animations'].items():
        action = bpy.data.actions.new('AN_Creature_' + kind + '_Blockout')
        action.use_fake_user = True
        action['loop'] = spec['loop']
        action['state'] = spec['state']
        action['root_motion'] = False
        rig.animation_data.action = action
        for frame in range(1, spec['frames'] + 1):
            scene.frame_set(frame)
            apply_pose(rig, body, kind, (frame - 1) / (spec['frames'] - 1))
            for pose_bone in rig.pose.bones:
                pose_bone.keyframe_insert('rotation_quaternion', frame=frame, group=pose_bone.name)
                pose_bone.keyframe_insert('location', frame=frame, group=pose_bone.name)
        actions[kind] = action
    rig.animation_data.action = None
    clear_pose(rig)
    scene.frame_set(1)
    return actions


def export(rig, meshes, actions):
    files = []

    def write(name, selected, animate=False):
        bpy.ops.object.select_all(action='DESELECT')
        for obj in selected:
            obj.hide_set(False)
            obj.select_set(True)
        bpy.context.view_layer.objects.active = rig
        path = OUT / (name + '.fbx')
        source_name = rig.name
        # UE 5.7 strips Blender's "Armature" dummy node, which keeps `root` as the real
        # root in Unreal. The authored skeleton keeps its own name in the source file.
        rig.name = 'Armature'
        try:
            result = bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
                object_types={'MESH', 'ARMATURE'}, global_scale=1, apply_unit_scale=True,
                apply_scale_options='FBX_SCALE_NONE', axis_forward='-Y', axis_up='Z',
                add_leaf_bones=False, use_armature_deform_only=False,
                bake_anim=animate, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                bake_anim_simplify_factor=0, bake_anim_step=1, mesh_smooth_type='FACE')
        finally:
            rig.name = source_name
        if 'FINISHED' not in result:
            raise RuntimeError('Export failed ' + name)
        files.append({'id': name, 'path': path.relative_to(ROOT).as_posix(),
                      'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})

    rig.animation_data.action = None
    clear_pose(rig)
    bpy.context.view_layer.update()
    for mesh in meshes:
        for other in meshes:
            other.hide_set(other != mesh)
        write(mesh.name, [rig, mesh])
    for other in meshes:
        other.hide_set(False)
    for kind, action in actions.items():
        rig.animation_data.action = action
        bpy.context.scene.frame_start = 1
        bpy.context.scene.frame_end = CFG['animations'][kind]['frames']
        bpy.context.scene.frame_set(1)
        write(action.name, [rig], True)
    rig.animation_data.action = actions['Graze']
    return files


def roundtrip(files):
    results = []
    expected_bones = set(BONES)
    for item in files:
        for obj in bpy.context.scene.objects:
            obj.hide_set(False)
        reset_scene()
        bpy.ops.import_scene.fbx(filepath=str(ROOT / item['path']))
        rigs = [o for o in bpy.context.scene.objects if o.type == 'ARMATURE']
        meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
        assert len(rigs) == 1, item['id']
        rig = rigs[0]
        assert set(b.name for b in rig.data.bones) == expected_bones, 'Hierarchy lost in FBX'
        for name, (_, _, parent) in BONES.items():
            actual = rig.data.bones[name].parent.name if rig.data.bones[name].parent else None
            assert actual == parent, 'Parenting lost for ' + name
        row = {'id': item['id'], 'passed': True, 'bones': len(expected_bones)}
        if item['id'].startswith('SK_'):
            assert len(meshes) == 1, [o.name for o in meshes]
            bpy.context.view_layer.update()
            corners = [meshes[0].matrix_world @ v.co for v in meshes[0].data.vertices]
            length = max(c.y for c in corners) - min(c.y for c in corners)
            assert abs(length - CFG['length_m']) < .06, length
            assert any(m.type == 'ARMATURE' for m in meshes[0].modifiers), 'Skin lost in FBX'
            assert all(v.groups for v in meshes[0].data.vertices), 'Weights lost in FBX'
            row['length_m'] = length
        else:
            assert not meshes, 'Animation FBX must carry no mesh'
            assert rig.animation_data and rig.animation_data.action
            action = rig.animation_data.action
            kind = item['id'].removeprefix('AN_Creature_').removesuffix('_Blockout')
            frames = CFG['animations'][kind]['frames']
            assert abs(action.frame_range[1] - action.frame_range[0] - (frames - 1)) < .01
            samples = []
            for frame in [1, 1 + (frames - 1) // 4]:
                bpy.context.scene.frame_set(frame)
                samples.append([tuple(v for row_values in b.matrix for v in row_values)
                                for b in rig.pose.bones])
            assert samples[0] != samples[1], 'Static exported animation'
            row['duration_frames'] = frames
        results.append(row)
    return results


def setup_preview(rig):
    scene = bpy.context.scene
    bpy.ops.mesh.primitive_plane_add(size=200, location=(0, 0, -.004))
    floor = bpy.context.object
    floor.name = 'Preview_Floor_DoNotExport'
    floor.data.materials.append(material('M_PreviewFloor', (.045, .058, .072)))
    bpy.ops.object.camera_add(location=(2.9, -2.4, 1.35))
    camera = bpy.context.object
    camera.rotation_euler = (Vector((0, -.05, .36)) - camera.location).to_track_quat('-Z', 'Y').to_euler()
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = 2.05
    scene.camera = camera
    for location, power, size in [((2, -3, 4), 500, 3), ((-3, -1, 2), 300, 3), ((1, 3, 3), 700, 2)]:
        bpy.ops.object.light_add(type='AREA', location=location)
        lamp = bpy.context.object
        lamp.data.energy = power
        lamp.data.shape = 'DISK'
        lamp.data.size = size
        lamp.rotation_euler = (Vector((0, 0, .4)) - lamp.location).to_track_quat('-Z', 'Y').to_euler()
    scene.world.color = (.15, .15, .15)
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 24
    scene.cycles.device = 'CPU'
    scene.render.resolution_x = 1100
    scene.render.resolution_y = 760
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    return camera


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    report = {'passed': False, 'generator_version': CFG['generator_version'],
              'blender_version': bpy.app.version_string,
              'species_id': CFG['species_id'],
              'config_sha256': hashlib.sha256(CONFIG_PATH.read_bytes()).hexdigest(),
              'generator_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'validator_sha256': hashlib.sha256((TOOL_ROOT / 'validators/creature.py').read_bytes()).hexdigest(),
              'unreal_gameplay_integration': False}
    try:
        reset_scene()
        rig = make_rig()
        make_parts()

        def parts_signature():
            data = [([[round(c, 7) for c in v] for v in vertices], faces, weights, mat, plated)
                    for vertices, faces, weights, mat, plated in PARTS]
            return hashlib.sha256(json.dumps(data, sort_keys=True).encode()).hexdigest()

        first_signature = parts_signature()
        make_parts()
        assert first_signature == parts_signature(), 'Non-deterministic authored geometry'
        report['determinism'] = {'passed': True, 'geometry_sha256': first_signature}

        meshes = [make_mesh(spec, rig) for spec in CFG['meshes']]
        report['source'] = validate_source(rig, meshes, CFG)
        report['negative_controls'] = negative_controls(rig, meshes, CFG)
        actions = make_actions(rig, meshes[0])
        report['animations'] = validate_actions(rig, meshes[0], meshes, actions, CFG)
        report['files'] = export(rig, meshes, actions)

        rig.animation_data.action = actions['Walk']
        bpy.context.scene.frame_start, bpy.context.scene.frame_end = 1, CFG['animations']['Walk']['frames']
        bpy.context.scene.frame_set(1)
        bpy.ops.object.select_all(action='DESELECT')
        rig.select_set(True)
        bpy.context.view_layer.objects.active = rig
        source_path = OUT / 'Creature_Blockout_Source.blend'
        bpy.ops.wm.save_as_mainfile(filepath=str(source_path))
        report['roundtrip'] = roundtrip(report['files'])

        bpy.ops.wm.open_mainfile(filepath=str(source_path))
        rig = bpy.data.objects[RIG_NAME]
        variant = bpy.data.objects[CFG['meshes'][1]['id']]
        base = bpy.data.objects[CFG['meshes'][0]['id']]
        variant.hide_set(True)
        variant.hide_render = True
        camera = setup_preview(rig)
        for kind, spec in CFG['animations'].items():
            rig.animation_data.action = bpy.data.actions['AN_Creature_' + kind + '_Blockout']
            bpy.context.scene.frame_set(max(1, spec['frames'] // 2))
            bpy.context.scene.render.filepath = str(OUT / ('Preview_' + kind + '.png'))
            bpy.ops.render.render(write_still=True)
        # La variante se juzga en reposo, contra la base, no en mitad de un ciclo.
        rig.animation_data.action = None
        clear_pose(rig)
        base.hide_set(True)
        base.hide_render = True
        variant.hide_set(False)
        variant.hide_render = False
        bpy.context.view_layer.update()
        bpy.context.scene.render.filepath = str(OUT / 'Preview_Variant.png')
        bpy.ops.render.render(write_still=True)
        base.hide_set(False)
        base.hide_render = False
        variant.hide_set(True)
        variant.hide_render = True
        bpy.context.scene.render.filepath = str(OUT / 'Preview_Base.png')
        bpy.ops.render.render(write_still=True)

        variant.hide_set(False)
        variant.hide_render = False
        rig.animation_data.action = bpy.data.actions['AN_Creature_Walk_Blockout']
        bpy.context.scene.frame_start, bpy.context.scene.frame_end = 1, CFG['animations']['Walk']['frames']
        bpy.context.scene.frame_set(1)
        bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'Creature_Blockout_Preview.blend'))
        report.update(passed=True, stage='source_skin_actions_fbx_preview_complete')
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        REPORT.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('ASTRAEON_CREATURE: PASS')


if __name__ == '__main__':
    main()
