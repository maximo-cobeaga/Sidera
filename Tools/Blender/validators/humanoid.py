"""Contract tests for rigged prototypes, separate from the static-mesh validator."""
import math
import bpy
import bmesh


def validate_source(rig, body, hands, config):
    bones = rig.data.bones
    assert [b.name for b in bones if not b.parent] == ['root'], 'One ground root required'
    for name in ['socket_tool_l', 'socket_tool_r', 'socket_backpack', 'socket_helmet']:
        assert name in bones, 'Missing attachment: ' + name
    results = []
    for obj, budget, materials in [(body, config['body_triangle_budget'], 4),
                                    (hands, config['hands_triangle_budget'], 2)]:
        assert tuple(obj.scale) == (1, 1, 1), 'Unapplied mesh scale'
        assert len(obj.data.materials) <= materials
        assert len(obj.data.uv_layers) == 1
        assert obj.parent == rig
        assert any(m.type == 'ARMATURE' and m.object == rig for m in obj.modifiers)
        for vertex in obj.data.vertices:
            assert all(math.isfinite(c) for c in vertex.co)
            assert 1 <= len(vertex.groups) <= 4, 'Unweighted vertex or too many influences'
            assert abs(sum(g.weight for g in vertex.groups) - 1) < 1e-5, 'Weights not normalized'
            assert all(obj.vertex_groups[g.group].name in bones for g in vertex.groups), 'Unknown bone'
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        try:
            assert all(e.is_manifold for e in bm.edges), 'Open suit shell'
            assert all(f.calc_area() > 1e-10 for f in bm.faces), 'Degenerate polygon'
        finally:
            bm.free()
        obj.data.calc_loop_triangles()
        tris = len(obj.data.loop_triangles)
        assert tris <= budget, 'Triangle budget'
        results.append({'id': obj.name, 'vertices': len(obj.data.vertices), 'triangles': tris,
                        'materials': len(obj.data.materials), 'passed': True})
    height = max(v.co.z for v in body.data.vertices) - min(v.co.z for v in body.data.vertices)
    assert abs(height - config['height_m']) < .001, 'Canonical height'
    return {'bones': len(bones), 'height_m': height, 'meshes': results, 'passed': True}


def negative_controls(rig, body, hands, config):
    results = []
    def rejects(label, corrupt, restore):
        corrupt()
        rejected = False
        try:
            validate_source(rig, body, hands, config)
        except AssertionError:
            rejected = True
        finally:
            restore()
        assert rejected, 'Validator accepted ' + label
        results.append({'case': label, 'rejected': True})
    rejects('unapplied_scale', lambda: setattr(hands, 'scale', (2, 1, 1)),
            lambda: setattr(hands, 'scale', (1, 1, 1)))
    g = body.vertex_groups[body.data.vertices[0].groups[0].group]
    old = g.weight(0)
    rejects('unnormalized_weights', lambda: g.add([0], .25, 'REPLACE'),
            lambda: g.add([0], old, 'REPLACE'))
    rejects('unweighted_vertex', lambda: g.remove([0]), lambda: g.add([0], old, 'REPLACE'))
    socket = rig.data.bones['socket_tool_r']
    rejects('missing_tool_attachment', lambda: setattr(socket, 'name', 'bad_attachment'),
            lambda: setattr(socket, 'name', 'socket_tool_r'))
    return results


def validate_actions(rig, body, hands, actions, config):
    results = []
    for kind, action in actions.items():
        rig.animation_data.action = action
        end = config['animations'][kind]['frames']
        matrices = []
        lowest = float('inf')
        highest = float('-inf')
        for frame in range(1, end + 1):
            bpy.context.scene.frame_set(frame)
            bpy.context.view_layer.update()
            root = rig.pose.bones['root']
            assert root.location.length < 1e-7, 'Unexpected root motion'
            assert abs(root.rotation_quaternion.w - 1) < 1e-6
            matrices.append(tuple(round(v, 6) for pb in rig.pose.bones for row in pb.matrix for v in row))
            graph = bpy.context.evaluated_depsgraph_get()
            for obj in [body, hands]:
                evaluated = obj.evaluated_get(graph)
                mesh = evaluated.to_mesh()
                try:
                    assert all(math.isfinite(c) for v in mesh.vertices for c in v.co), 'Non-finite skin deformation'
                    assert all(v.co.length < 3 for v in mesh.vertices), 'Exploding skin'
                    if obj == body:
                        if kind in ('Walk', 'Run', 'Land'):
                            assert abs(min(v.co.z for v in mesh.vertices)) < .001, 'Boot ground contact'
                        lowest = min(lowest, min(v.co.z for v in mesh.vertices))
                        highest = max(highest, max(v.co.z for v in mesh.vertices))
                finally:
                    evaluated.to_mesh_clear()
        assert len(set(matrices)) > 3, 'Animation contains no meaningful movement'
        if config['animations'][kind]['loop']:
            assert matrices[0] == matrices[-1], 'Loop endpoint discontinuity'
        results.append({'id': action.name, 'frames': end, 'loop': config['animations'][kind]['loop'],
                        'root_motion': False, 'min_z_m': lowest, 'max_z_m': highest, 'passed': True})
    rig.animation_data.action = None
    return results
