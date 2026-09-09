"""Contract tests for the quadruped prototype.

Separate from validators/humanoid.py on purpose: the humanoid module's sha256 is an input
to an already-approved validation report, so it stays frozen. The rules also differ — a
quadruped has four ground contacts instead of two, and its body legitimately drops to the
floor during the death clip.
"""
import math
import bpy
import bmesh

REQUIRED_SOCKETS = ['socket_plate_back', 'socket_horn_l', 'socket_horn_r', 'socket_sensor']
# Interfaces modulares del plan maestro: cuello, pelvis, hombros y raíz de cola.
REQUIRED_INTERFACES = ['neck_01', 'pelvis', 'shoulder_l', 'shoulder_r', 'tail_01']
GROUND_TOLERANCE_M = 0.0015


def validate_source(rig, meshes, config):
    bones = rig.data.bones
    assert [b.name for b in bones if not b.parent] == ['root'], 'One ground root required'
    for name in REQUIRED_SOCKETS + REQUIRED_INTERFACES:
        assert name in bones, 'Missing attachment or interface: ' + name
    # Cuatro cadenas de tres articulaciones: sin esto no hay marcha creíble ni IK futuro.
    for prefix in ['upperleg_f', 'lowerleg_f', 'foot_f', 'upperleg_b', 'lowerleg_b', 'foot_b']:
        for side in ['l', 'r']:
            assert prefix + '_' + side in bones, 'Missing limb joint: ' + prefix + '_' + side
    assert 'jaw' in bones, 'Missing jaw'
    assert [f'tail_{i:02}' in bones for i in range(1, 5)] == [True] * 4, 'Tail needs four segments'

    results = []
    for obj in meshes:
        assert tuple(obj.scale) == (1, 1, 1), 'Unapplied mesh scale'
        assert tuple(obj.location) == (0, 0, 0), 'Displaced mesh origin'
        assert len(obj.data.materials) <= config['material_slots_per_mesh'], 'Material slot budget'
        assert len(obj.data.uv_layers) == 1, 'Exactly one UV channel'
        assert obj.parent == rig, 'Mesh is not parented to the rig'
        assert any(m.type == 'ARMATURE' and m.object == rig for m in obj.modifiers), 'Missing skin'
        for vertex in obj.data.vertices:
            assert all(math.isfinite(c) for c in vertex.co), 'Non-finite vertex'
            assert 1 <= len(vertex.groups) <= 4, 'Unweighted vertex or too many influences'
            assert abs(sum(g.weight for g in vertex.groups) - 1) < 1e-5, 'Weights not normalized'
            assert all(obj.vertex_groups[g.group].name in bones for g in vertex.groups), 'Unknown bone'
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        try:
            assert all(f.calc_area() > 1e-10 for f in bm.faces), 'Degenerate polygon'
        finally:
            bm.free()
        obj.data.calc_loop_triangles()
        tris = len(obj.data.loop_triangles)
        assert tris <= config['triangle_budget'], 'Triangle budget'

        low = [min(v.co[i] for v in obj.data.vertices) for i in range(3)]
        high = [max(v.co[i] for v in obj.data.vertices) for i in range(3)]
        dims = [b - a for a, b in zip(low, high)]
        # La silueta tiene que leerse baja y ancha, no como un perro alto ni una serpiente.
        assert abs(low[2]) < GROUND_TOLERANCE_M, 'Rest pose does not stand on the ground'
        assert abs(dims[1] - config['length_m']) < 0.06, 'Length is off the design silhouette'
        assert dims[0] <= config['width_m'] + 0.02, 'Wider than the design silhouette'
        assert abs(high[2] - config['shoulder_height_m']) < 0.05, 'Shoulder height is off'
        assert dims[1] > dims[2] * 1.6, 'Silhouette is not low and wide'
        results.append({'id': obj.name, 'vertices': len(obj.data.vertices), 'triangles': tris,
                        'materials': len(obj.data.materials), 'dimensions_m': dims, 'passed': True})
    return {'bones': len(bones), 'meshes': results, 'passed': True}


def negative_controls(rig, meshes, config):
    results = []

    def rejects(label, corrupt, restore):
        corrupt()
        rejected = False
        try:
            validate_source(rig, meshes, config)
        except AssertionError:
            rejected = True
        finally:
            restore()
        assert rejected, 'Validator accepted ' + label
        results.append({'case': label, 'rejected': True})

    body = meshes[0]
    rejects('unapplied_scale', lambda: setattr(body, 'scale', (2, 1, 1)),
            lambda: setattr(body, 'scale', (1, 1, 1)))
    rejects('displaced_origin', lambda: setattr(body, 'location', (0, 0, .2)),
            lambda: setattr(body, 'location', (0, 0, 0)))
    group = body.vertex_groups[body.data.vertices[0].groups[0].group]
    original = group.weight(0)
    rejects('unnormalized_weights', lambda: group.add([0], .25, 'REPLACE'),
            lambda: group.add([0], original, 'REPLACE'))
    rejects('unweighted_vertex', lambda: group.remove([0]),
            lambda: group.add([0], original, 'REPLACE'))
    socket = rig.data.bones['socket_horn_r']
    rejects('missing_socket', lambda: setattr(socket, 'name', 'bad_socket'),
            lambda: setattr(socket, 'name', 'socket_horn_r'))
    tail = rig.data.bones['tail_04']
    rejects('truncated_tail', lambda: setattr(tail, 'name', 'tail_extra'),
            lambda: setattr(tail, 'name', 'tail_04'))
    return results


def validate_actions(rig, body, meshes, actions, config):
    """Every frame of every clip has to keep the animal finite, intact and on the ground."""
    results = []
    for kind, action in actions.items():
        rig.animation_data.action = action
        spec = config['animations'][kind]
        matrices = []
        lowest = float('inf')
        highest = float('-inf')
        for frame in range(1, spec['frames'] + 1):
            bpy.context.scene.frame_set(frame)
            bpy.context.view_layer.update()
            root = rig.pose.bones['root']
            assert root.location.length < 1e-7, 'Unexpected root motion'
            assert abs(root.rotation_quaternion.w - 1) < 1e-6, 'Unexpected root rotation'
            matrices.append(tuple(round(v, 6) for pb in rig.pose.bones for row in pb.matrix for v in row))
            graph = bpy.context.evaluated_depsgraph_get()
            for obj in meshes:
                evaluated = obj.evaluated_get(graph)
                mesh = evaluated.to_mesh()
                try:
                    assert all(math.isfinite(c) for v in mesh.vertices for c in v.co), 'Non-finite skin deformation'
                    assert all(v.co.length < 3 for v in mesh.vertices), 'Exploding skin'
                    if obj == body:
                        floor = min(v.co.z for v in mesh.vertices)
                        # Nunca se hunde. Sí puede despegar: un galope tiene fase de vuelo,
                        # y el contrato es que el clip apoye en algún momento, no siempre.
                        assert floor > -GROUND_TOLERANCE_M, 'Creature sinks into the ground in ' + kind
                        lowest = min(lowest, floor)
                        highest = max(highest, max(v.co.z for v in mesh.vertices))
                finally:
                    evaluated.to_mesh_clear()
        assert len(set(matrices)) > 3, 'Animation contains no meaningful movement'
        assert lowest < GROUND_TOLERANCE_M, 'Creature never touches the ground in ' + kind
        if spec['loop']:
            assert matrices[0] == matrices[-1], 'Loop endpoint discontinuity'
        results.append({'id': action.name, 'frames': spec['frames'], 'loop': spec['loop'],
                        'state': spec['state'], 'root_motion': False,
                        'min_z_m': lowest, 'max_z_m': highest, 'passed': True})

    # La muerte tiene que terminar más baja que el reposo, o no se lee como caída.
    death = next(r for r in results if r['id'].endswith('Death_Blockout'))
    rig.animation_data.action = actions['Death']
    bpy.context.scene.frame_set(config['animations']['Death']['frames'])
    bpy.context.view_layer.update()
    evaluated = body.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    try:
        peak = max((v.co.copy() for v in mesh.vertices), key=lambda c: c.z)
    finally:
        evaluated.to_mesh_clear()
    final_height = peak.z
    limit = config['shoulder_height_m'] * 0.72
    # El punto más alto se reporta entero: saber si lo que sobresale es el lomo, una pata
    # o la cabeza es la diferencia entre corregir la pose y adivinar otro ángulo.
    assert final_height < limit, ('The corpse never came down: peak %.3f m at (%.3f, %.3f, %.3f), '
                                  'limit %.3f m' % (final_height, peak.x, peak.y, peak.z, limit))
    death['final_height_m'] = final_height
    death['final_peak_m'] = [peak.x, peak.y, peak.z]

    rig.animation_data.action = None
    return results
