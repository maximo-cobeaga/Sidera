"""Original Q1 explorer, shared armature, FP gloves and in-place prototype actions.

Metres, +Z up, -Y forward. No external assets or humanoid retarget assumptions.
Run in a fresh background Blender process; only this generator's outputs are replaced.
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
from validators.humanoid import validate_source, validate_actions, negative_controls

OUT = ROOT / 'ContentPipeline/Generated/HumanoidBlockout'
REPORT = ROOT / 'ContentPipeline/reports/humanoid_blockout_validation.json'
CONFIG_PATH = TOOL_ROOT / 'configs/humanoid_blockout.json'
CFG = json.loads(CONFIG_PATH.read_text(encoding='utf-8'))
BONES = {}
PARTS = []


def bone(name, head, tail, parent=None):
    BONES[name] = (Vector(head), Vector(tail), parent)
    return name


def make_rig():
    BONES.clear()
    bone('root', (0, 0, 0), (0, 0, .16))
    bone('pelvis', (0, 0, .93), (0, 0, 1.06), 'root')
    bone('spine_01', (0, 0, 1.06), (0, 0, 1.19), 'pelvis')
    bone('spine_02', (0, 0, 1.19), (0, 0, 1.32), 'spine_01')
    bone('spine_03', (0, 0, 1.32), (0, 0, 1.44), 'spine_02')
    bone('neck', (0, 0, 1.44), (0, 0, 1.55), 'spine_03')
    bone('head', (0, 0, 1.55), (0, 0, 1.77), 'neck')
    for side, sign in [('l', 1), ('r', -1)]:
        p = lambda x, y, z: (x * sign, y, z)
        bone('clavicle_' + side, p(.04, 0, 1.42), p(.23, 0, 1.43), 'spine_03')
        bone('upperarm_' + side, p(.23, 0, 1.43), p(.385, 0, 1.205), 'clavicle_' + side)
        bone('lowerarm_' + side, p(.385, 0, 1.205), p(.505, 0, .995), 'upperarm_' + side)
        hand_start = Vector(p(.505, 0, .995))
        direction = Vector(p(.35, 0, -.937)).normalized()
        spread = Vector(p(.937, 0, .35)).normalized()
        bone('hand_' + side, hand_start, hand_start + direction * .09, 'lowerarm_' + side)
        for finger, offset, length in [('index', -.031, .078), ('middle', -.010, .085),
                                       ('ring', .013, .078), ('pinky', .034, .060)]:
            start = hand_start + direction * .081 + spread * offset
            parent = 'hand_' + side
            for segment, fraction in enumerate([.43, .33, .24], 1):
                end = start + direction * length * fraction
                parent = bone(f'{finger}_{segment:02}_{side}', start, end, parent)
                start = end
        start = hand_start + direction * .033 - spread * .04
        thumb_dir = (direction * .60 - spread * .8).normalized()
        parent = 'hand_' + side
        for segment, length in enumerate([.029, .025, .022], 1):
            end = start + thumb_dir * length
            parent = bone(f'thumb_{segment:02}_{side}', start, end, parent)
            start = end
        bone('thigh_' + side, p(.105, 0, .96), p(.105, -.015, .54), 'pelvis')
        bone('calf_' + side, p(.105, -.015, .54), p(.105, 0, .12), 'thigh_' + side)
        bone('foot_' + side, p(.105, 0, .12), p(.105, -.15, .065), 'calf_' + side)
        bone('ball_' + side, p(.105, -.15, .065), p(.105, -.23, .065), 'foot_' + side)
        bone('socket_tool_' + side, hand_start + direction * .05,
             hand_start + direction * .05 + Vector((0, -.06, 0)), 'hand_' + side)
    bone('socket_backpack', (0, .13, 1.30), (0, .20, 1.30), 'spine_03')
    bone('socket_helmet', (0, 0, 1.65), (0, 0, 1.75), 'head')
    data = bpy.data.armatures.new('SKEL_Humanoid_A')
    rig = bpy.data.objects.new('SKEL_Humanoid_A', data)
    bpy.context.collection.objects.link(rig)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    for name, (head, tail, parent) in BONES.items():
        eb = data.edit_bones.new(name)
        eb.head, eb.tail = head, tail
        eb.align_roll(Vector((0, -1, 0)))
        if parent:
            eb.parent = data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    rig.show_in_front = True
    rig.data.display_type = 'STICK'
    rig['generator_version'] = CFG['generator_version']
    rig['forward_axis'] = '-Y'
    rig['quality'] = 'Q1 prototype; no Manny/Quinn compatibility claim'
    return rig


def add_part(vertices, faces, weights, mat, fp=False):
    # bpy's UV sphere operator can enumerate identical polygons in different order.
    # Canonicalize polygon start/order without changing winding or topology.
    canonical = []
    for face in faces:
        f = tuple(face)
        start = f.index(min(f))
        canonical.append(f[start:] + f[:start])
    PARTS.append((vertices, sorted(canonical), weights, mat, fp))


def tube(chain, radii, mat='fabric', fp=False, sides=12):
    """Closed connected ring surface with blended weights at every joint.

    Near joints two rings distribute bend weights over a finite region. Caps use
    triangle fans, avoiding ambiguous concave cap triangulation after posing.
    """
    vertices, faces, weights = [], [], []
    for i, name in enumerate(chain):
        head, tail, _ = BONES[name]
        direction = (tail - head).normalized()
        depth = Vector((0, 1, 0))
        if abs(depth.dot(direction)) > .95:
            depth = Vector((1, 0, 0))
        across = direction.cross(depth).normalized()
        depth = across.cross(direction).normalized()
        for t in ([0, .22, .78] if i < len(chain) - 1 else [0, .22, .78, 1]):
            center = head.lerp(tail, t)
            width, thick = radii[i]
            taper = .87 if i == len(chain) - 1 and t == 1 else 1
            w = {name: 1.0}
            if t == 0 and i > 0:
                w = {chain[i-1]: .5, name: .5}
            elif t == .22 and i > 0:
                w = {chain[i-1]: .15, name: .85}
            elif t == .78 and i < len(chain) - 1:
                w = {name: .85, chain[i+1]: .15}
            for j in range(sides):
                angle = math.tau * j / sides
                vertices.append(center + taper * (across * width * math.cos(angle) + depth * thick * math.sin(angle)))
                weights.append(w)
    rings = len(vertices) // sides
    for ring in range(rings - 1):
        for j in range(sides):
            a, b = ring * sides + j, ring * sides + (j + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    for ring, reverse in [(0, True), (rings - 1, False)]:
        ids = list(range(ring * sides, (ring + 1) * sides))
        center = sum((vertices[k] for k in ids), Vector()) / sides
        ci = len(vertices)
        vertices.append(center)
        weights.append(weights[ids[0]])
        for j in range(sides):
            a, b = ids[j], ids[(j + 1) % sides]
            faces.append((ci, b, a) if reverse else (ci, a, b))
    add_part(vertices, faces, weights, mat, fp)


def ellipsoid(center, radius, name, mat, fp=False, segments=16, rings=10):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=center)
    obj = bpy.context.object
    verts = [Vector(center) + Vector(tuple(v.co[i] * radius[i] for i in range(3))) for v in obj.data.vertices]
    add_part(verts, [tuple(p.vertices) for p in obj.data.polygons], [{name: 1.0}] * len(verts), mat, fp)
    bpy.data.objects.remove(obj, do_unlink=True)


def box(center, scale, name, mat, fp=False, bevel=.012):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj = bpy.context.object
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    mod = obj.modifiers.new('Machined edges', 'BEVEL')
    mod.width, mod.segments = bevel, 2
    bpy.ops.object.modifier_apply(modifier=mod.name)
    add_part([obj.matrix_world @ v.co for v in obj.data.vertices],
             [tuple(p.vertices) for p in obj.data.polygons],
             [{name: 1.0}] * len(obj.data.vertices), mat, fp)
    bpy.data.objects.remove(obj, do_unlink=True)


def make_parts():
    PARTS.clear()
    tube(['pelvis', 'spine_01', 'spine_02', 'spine_03'],
         [(.15, .105), (.14, .10), (.175, .112), (.19, .115)])
    ellipsoid((0, 0, 1.06), (.17, .12, .11), 'pelvis', 'fabric')
    # Curved segmented chest plates and compact service pack.
    for x in [-.093, .093]:
        box((x, -.099, 1.325), (.16, .062, .17), 'spine_03', 'shell', bevel=.025)
    box((0, -.12, 1.23), (.115, .043, .065), 'spine_02', 'shell')
    box((0, -.147, 1.335), (.036, .012, .085), 'spine_03', 'accent', bevel=.004)
    box((0, .13, 1.28), (.27, .11, .29), 'spine_03', 'shell', bevel=.025)
    for x in [-.083, .083]:
        box((x, .198, 1.285), (.039, .035, .19), 'spine_03', 'fabric')
    tube(['neck'], [(.065, .065)])
    ellipsoid((0, 0, 1.65), (.128, .126, .15), 'head', 'shell', segments=24, rings=16)
    ellipsoid((0, -.087, 1.66), (.113, .062, .089), 'head', 'visor', segments=24, rings=16)
    box((0, -.102, 1.55), (.105, .068, .035), 'head', 'fabric')
    for side, sign in [('l', 1), ('r', -1)]:
        tube(['upperarm_' + side, 'lowerarm_' + side], [(.071, .074), (.052, .052)])
        # Separate FP sleeves use identical skeleton and source geometry.
        tube(['lowerarm_' + side], [(.053, .053)], fp='sleeve')
        ellipsoid((sign * .237, 0, 1.412), (.086, .085, .094), 'upperarm_' + side, 'shell')
        elbow = BONES['lowerarm_' + side][0]
        ellipsoid(elbow, (.055, .06, .055), 'lowerarm_' + side, 'fabric', fp=True)
        wrist = BONES['hand_' + side][0]
        ellipsoid(wrist, (.042, .04, .045), 'hand_' + side, 'shell', fp=True)
        tube(['hand_' + side], [(.047, .023)], fp=True)
        for finger in ['thumb', 'index', 'middle', 'ring', 'pinky']:
            chain = [f'{finger}_{i:02}_{side}' for i in [1, 2, 3]]
            r = .010 if finger != 'pinky' else .008
            tube(chain, [(r, r), (r * .9, r * .9), (r * .78, r * .78)], fp=True, sides=10)
            knuckle = BONES[chain[0]][0] + Vector((0, .009, 0))
            ellipsoid(knuckle, (.012, .009, .012), chain[0], 'shell', fp=True, segments=8, rings=6)
        tube(['thigh_' + side, 'calf_' + side], [(.091, .095), (.064, .064)])
        box((sign * .105, -.074, .56), (.12, .05, .125), 'calf_' + side, 'shell', bevel=.022)
        box((sign * .105, -.067, .285), (.095, .035, .21), 'calf_' + side, 'shell', bevel=.012)
        box((sign * .105, -.063, .078), (.151, .29, .146), 'foot_' + side, 'fabric', bevel=.025)
        box((sign * .105, -.073, .023), (.157, .31, .046), 'foot_' + side, 'shell', bevel=.01)


def make_mesh(name, rig, fp=False):
    verts, faces, weights, mats = [], [], [], []
    for pv, pf, pw, mat, is_fp in PARTS:
        if fp and not is_fp:
            continue
        # FP-only sleeve overlaps body sleeve: omit it from full-body asset.
        if not fp and is_fp == 'sleeve':
            continue
        offset = len(verts)
        verts.extend(pv)
        faces.extend(tuple(offset + k for k in f) for f in pf)
        weights.extend(pw)
        mats.extend([mat] * len(pf))
    mesh = bpy.data.meshes.new(name + '_Geometry')
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    palette = ['fabric', 'shell'] if fp else list(CFG['palette'])
    for key in palette:
        mesh.materials.append(material('M_Human_' + key, CFG['palette'][key]))
    for poly, key in zip(mesh.polygons, mats):
        poly.material_index = palette.index(key)
        poly.use_smooth = True
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    for key in BONES:
        group = obj.vertex_groups.new(name=key)
        for i, w in enumerate(weights):
            if key in w:
                group.add([i], w[key], 'REPLACE')
    obj.parent = rig
    mod = obj.modifiers.new('Shared humanoid deformation', 'ARMATURE')
    mod.object = rig
    obj['quality'] = 'Q1; original procedural prototype, overlapping suit shells'
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(island_margin=.015)
    bpy.ops.object.mode_set(mode='OBJECT')
    return obj


def clear_pose(rig):
    for pb in rig.pose.bones:
        pb.rotation_mode = 'QUATERNION'
        pb.rotation_quaternion = (1, 0, 0, 0)
        pb.location = (0, 0, 0)
        pb.scale = (1, 1, 1)


def rotate(rig, name, axis, degrees):
    pb = rig.pose.bones[name]
    rest = pb.bone.matrix_local.to_quaternion()
    pb.rotation_quaternion = rest.inverted() @ Quaternion(Vector(axis), math.radians(degrees)) @ rest


def pose(rig, kind, t):
    clear_pose(rig)
    cycle = math.sin(math.tau * t)
    envelope = math.sin(math.pi * t) ** 2
    # Rest arms hang 12 degrees off torso instead of the export A-pose.
    for side, sign in [('l', 1), ('r', -1)]:
        rotate(rig, 'clavicle_' + side, (0, 1, 0), sign * 21)
    if kind == 'Idle':
        rotate(rig, 'spine_02', (1, 0, 0), .8 * cycle)
        rotate(rig, 'head', (0, 0, 1), 1.5 * cycle)
    elif kind in ('Walk', 'Run'):
        running = kind == 'Run'
        amplitude = 37 if running else 24
        rig.pose.bones['pelvis'].location.y = (.028 if running else .012) * (1 - math.cos(2 * math.tau * t))
        rotate(rig, 'spine_02', (1, 0, 0), 8 if running else 2)
        for side, sign in [('l', 1), ('r', -1)]:
            swing = cycle * sign
            rotate(rig, 'thigh_' + side, (1, 0, 0), -amplitude * swing)
            rotate(rig, 'calf_' + side, (1, 0, 0), (65 if running else 34) * max(0, swing))
            rotate(rig, 'foot_' + side, (1, 0, 0), -12 * max(0, swing))
            rotate(rig, 'upperarm_' + side, (1, 0, 0), amplitude * .75 * swing)
            rotate(rig, 'lowerarm_' + side, (1, 0, 0), -65 if running else -15)
    elif kind in ('Jump', 'Land'):
        bend = envelope * (35 if kind == 'Land' else 26)
        rig.pose.bones['pelvis'].location.y = -envelope * .10
        for side in ['l', 'r']:
            rotate(rig, 'thigh_' + side, (1, 0, 0), -bend)
            rotate(rig, 'calf_' + side, (1, 0, 0), bend * 2)
            rotate(rig, 'foot_' + side, (1, 0, 0), -bend)
            rotate(rig, 'upperarm_' + side, (1, 0, 0), -envelope * (60 if kind == 'Jump' else 20))
        rotate(rig, 'spine_02', (1, 0, 0), envelope * 12)
    elif kind in ('Interact', 'Grip'):
        rotate(rig, 'upperarm_r', (1, 0, 0), -55 * envelope)
        rotate(rig, 'lowerarm_r', (1, 0, 0), -30 * envelope)
        for side in ['l', 'r']:
            for finger in ['thumb', 'index', 'middle', 'ring', 'pinky']:
                for segment in [1, 2, 3]:
                    name = f'{finger}_{segment:02}_{side}'
                    # Local X curls toward palm (-Y), independent of handedness.
                    angle = (48 if finger == 'thumb' else 65) * envelope
                    if kind == 'Interact':
                        angle *= .20
                    rig.pose.bones[name].rotation_quaternion = Quaternion((1, 0, 0), math.radians(angle))


def make_actions(rig, body):
    rig.animation_data_create()
    actions = {}
    scene = bpy.context.scene
    scene.render.fps = CFG['fps']
    for kind, spec in CFG['animations'].items():
        action = bpy.data.actions.new('AN_Human_' + kind + '_Blockout')
        action.use_fake_user = True
        action['loop'] = spec['loop']
        action['root_motion'] = False
        rig.animation_data.action = action
        for frame in range(1, spec['frames'] + 1):
            scene.frame_set(frame)
            pose(rig, kind, (frame - 1) / (spec['frames'] - 1))
            if kind in ('Walk', 'Run', 'Land'):
                # Ground the lower boot in the authored in-place clip. Runtime foot IK
                # and speed matching remain separate; this does not move the root.
                bpy.context.view_layer.update()
                evaluated = body.evaluated_get(bpy.context.evaluated_depsgraph_get())
                mesh = evaluated.to_mesh()
                try:
                    min_z = min(v.co.z for v in mesh.vertices)
                finally:
                    evaluated.to_mesh_clear()
                rig.pose.bones['pelvis'].location.y -= min_z
            for pb in rig.pose.bones:
                pb.keyframe_insert('rotation_quaternion', frame=frame, group=pb.name)
                pb.keyframe_insert('location', frame=frame, group=pb.name)
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
        # UE 5.7 FbxMainImport.cpp explicitly strips Blender's "Armature" dummy
        # node. Keep the authored skeleton name, but preserve a single root in UE.
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
        write(mesh.name, [rig, mesh])
    for kind, action in actions.items():
        rig.animation_data.action = action
        bpy.context.scene.frame_start = 1
        bpy.context.scene.frame_end = CFG['animations'][kind]['frames']
        bpy.context.scene.frame_set(1)
        write(action.name, [rig], True)
    rig.animation_data.action = actions['Idle']
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
            assert (rig.data.bones[name].parent.name if rig.data.bones[name].parent else None) == parent
        row = {'id': item['id'], 'passed': True, 'bones': len(expected_bones)}
        if item['id'].startswith('SK_'):
            assert len(meshes) == 1, [o.name for o in meshes]
            bpy.context.view_layer.update()
            z = [float((meshes[0].matrix_world @ v.co).z) for v in meshes[0].data.vertices]
            if 'Body' in item['id']:
                assert abs(max(z) - min(z) - CFG['height_m']) < .001, z
            assert any(m.type == 'ARMATURE' for m in meshes[0].modifiers)
            assert all(v.groups for v in meshes[0].data.vertices)
            row['height_m'] = max(z) - min(z)
        else:
            assert not meshes
            assert rig.animation_data and rig.animation_data.action
            action = rig.animation_data.action
            kind = item['id'].removeprefix('AN_Human_').removesuffix('_Blockout')
            assert abs(action.frame_range[1] - action.frame_range[0] - (CFG['animations'][kind]['frames'] - 1)) < .01
            samples = []
            for frame in [1, 1 + (CFG['animations'][kind]['frames'] - 1) // 4]:
                bpy.context.scene.frame_set(frame)
                samples.append([tuple(v for row in b.matrix for v in row) for b in rig.pose.bones])
            assert samples[0] != samples[1], 'Static exported animation'
            row['duration_frames'] = CFG['animations'][kind]['frames']
        results.append(row)
    return results


def setup_preview(rig, body, hands):
    hands.hide_render = True
    hands.hide_set(True)
    scene = bpy.context.scene
    bpy.ops.mesh.primitive_plane_add(size=200, location=(0, 0, -.025))
    floor = bpy.context.object
    floor.name = 'Preview_Floor_DoNotExport'
    floor.data.materials.append(material('M_PreviewFloor', (.045, .058, .072)))
    bpy.ops.object.camera_add(location=(2.5, -4.8, 2.2))
    camera = bpy.context.object
    camera.rotation_euler = (Vector((0, 0, .95)) - camera.location).to_track_quat('-Z', 'Y').to_euler()
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = 2.35
    scene.camera = camera
    for location, power, size in [((2,-3,4), 500, 3), ((-3,-1,2), 300, 3), ((1,3,3), 700, 2)]:
        bpy.ops.object.light_add(type='AREA', location=location)
        lamp = bpy.context.object
        lamp.data.energy = power
        lamp.data.shape = 'DISK'
        lamp.data.size = size
        lamp.rotation_euler = (Vector((0,0,1)) - lamp.location).to_track_quat('-Z','Y').to_euler()
    scene.world.color = (.15, .15, .15)
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 24
    scene.cycles.device = 'CPU'
    scene.render.resolution_x = 900
    scene.render.resolution_y = 1000
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    return camera


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    report = {'passed': False, 'generator_version': CFG['generator_version'],
              'blender_version': bpy.app.version_string,
              'config_sha256': hashlib.sha256(CONFIG_PATH.read_bytes()).hexdigest(),
              'generator_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'validator_sha256': hashlib.sha256((TOOL_ROOT / 'validators/humanoid.py').read_bytes()).hexdigest(),
              'unreal_gameplay_integration': False}
    try:
        reset_scene()
        rig = make_rig()
        make_parts()
        def parts_signature():
            data = [([[round(c, 7) for c in v] for v in verts], faces, weights, mat, fp)
                    for verts, faces, weights, mat, fp in PARTS]
            return hashlib.sha256(json.dumps(data, sort_keys=True).encode()).hexdigest()
        first_signature = parts_signature()
        make_parts()
        assert first_signature == parts_signature(), 'Non-deterministic authored geometry'
        report['determinism'] = {'passed': True, 'geometry_sha256': first_signature}
        body = make_mesh('SK_Human_Body_Blockout', rig)
        hands = make_mesh('SK_Human_HandsFP_Blockout', rig, True)
        report['source'] = validate_source(rig, body, hands, CFG)
        report['negative_controls'] = negative_controls(rig, body, hands, CFG)
        actions = make_actions(rig, body)
        report['animations'] = validate_actions(rig, body, hands, actions, CFG)
        report['files'] = export(rig, [body, hands], actions)
        hands.hide_set(True)
        hands.hide_render = True
        rig.animation_data.action = actions['Walk']
        bpy.context.scene.frame_start, bpy.context.scene.frame_end = 1, 31
        bpy.context.scene.frame_set(1)
        bpy.ops.object.select_all(action='DESELECT')
        rig.select_set(True)
        bpy.context.view_layer.objects.active = rig
        source_path = OUT / 'Humanoid_Blockout_Source.blend'
        bpy.ops.wm.save_as_mainfile(filepath=str(source_path))
        report['roundtrip'] = roundtrip(report['files'])
        bpy.ops.wm.open_mainfile(filepath=str(source_path))
        rig = bpy.data.objects['SKEL_Humanoid_A']
        body = bpy.data.objects['SK_Human_Body_Blockout']
        hands = bpy.data.objects['SK_Human_HandsFP_Blockout']
        camera = setup_preview(rig, body, hands)
        for kind, frame in [('Idle', 1), ('Walk', 8), ('Run', 6)]:
            rig.animation_data.action = bpy.data.actions['AN_Human_' + kind + '_Blockout']
            bpy.context.scene.frame_set(frame)
            bpy.context.scene.render.filepath = str(OUT / ('Preview_' + kind + '.png'))
            bpy.ops.render.render(write_still=True)
        rig.animation_data.action = bpy.data.actions['AN_Human_Walk_Blockout']
        bpy.context.scene.frame_start, bpy.context.scene.frame_end = 1, 31
        bpy.context.scene.frame_set(1)
        bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'Humanoid_Blockout_Preview.blend'))
        body.hide_render = True
        hands.hide_set(False)
        hands.hide_render = False
        rig.animation_data.action = None
        clear_pose(rig)
        bpy.context.view_layer.update()
        camera.location = (.9, -1.5, 1.17)
        camera.rotation_euler = (Vector((.51, 0, 1.015)) - camera.location).to_track_quat('-Z', 'Y').to_euler()
        camera.data.ortho_scale = .52
        bpy.context.scene.render.filepath = str(OUT / 'Preview_Hands_Open.png')
        bpy.ops.render.render(write_still=True)
        # Curl test keeps the arm in export pose for a directly comparable close-up.
        pose(rig, 'Grip', .5)
        for name in ['clavicle_l', 'clavicle_r', 'upperarm_r', 'lowerarm_r']:
            rig.pose.bones[name].rotation_quaternion = (1, 0, 0, 0)
        bpy.context.view_layer.update()
        bpy.context.scene.render.filepath = str(OUT / 'Preview_Hands_Grip.png')
        bpy.ops.render.render(write_still=True)
        report.update(passed=True, stage='source_skin_actions_fbx_preview_complete')
    except Exception as exc:
        report['error'] = str(exc)
        raise
    finally:
        REPORT.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('ASTRAEON_HUMANOID: PASS')


if __name__ == '__main__':
    main()
