"""Fail-closed static blockout checks; rig/texture production needs separate profiles."""
import math
import bmesh
from mathutils import Vector


def validate(obj, spec):
    errors = []
    def check(condition, reason):
        if not condition:
            errors.append(reason)
    check(obj.name == spec['id'], 'object_name')
    check(obj.type == 'MESH', 'mesh_type')
    if obj.type != 'MESH':
        return {'id': spec['id'], 'passed': False, 'errors': errors}
    check(all(abs(v - 1) < 1e-6 for v in obj.scale), 'applied_scale')
    check(all(abs(v) < 1e-6 for v in obj.rotation_euler), 'applied_rotation')
    check(obj.location.length < 1e-6, 'origin')
    check(not obj.modifiers, 'unsupported_modifiers')
    check(obj.parent is None, 'unexpected_parent_or_armature')
    check(all(math.isfinite(c) for v in obj.data.vertices for c in v.co), 'finite_vertices')
    corners = [Vector(c) for c in obj.bound_box]
    low = [min(v[i] for v in corners) for i in range(3)]
    high = [max(v[i] for v in corners) for i in range(3)]
    dims = [high[i] - low[i] for i in range(3)]
    check(all(abs(dims[i] - spec['dimensions_m'][i]) < 1e-4 for i in range(3)), 'dimensions_m')
    check(abs(low[2]) < 1e-5 and all(abs(low[i] + high[i]) < 1e-5 for i in (0, 1)), 'ground_pivot_xy_center')
    check(len(obj.data.uv_layers) > 0, 'missing_debug_uv')
    check(bool(obj.data.polygons), 'empty_mesh')
    check(all(3 <= len(p.vertices) <= 4 for p in obj.data.polygons), 'ngons')
    check(all(p.area > 1e-9 for p in obj.data.polygons), 'degenerate_faces')
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    check(tris <= spec['triangle_budget'], 'triangle_budget')
    check(len(obj.data.materials) == 1 and obj.data.materials[0] is not None
          and obj.data.materials[0].name == spec['material'], 'material_slots')
    check(all(p.material_index == 0 for p in obj.data.polygons), 'material_indices')
    # This blockout profile permits only flat procedural materials, no texture dependencies.
    check(not any(n.type == 'TEX_IMAGE' for m in obj.data.materials if m and m.use_nodes
                  for n in m.node_tree.nodes), 'unexpected_texture_dependency')
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    check(all(e.is_manifold and e.is_contiguous for e in bm.edges), 'manifold_or_winding')
    check(all(v.link_faces for v in bm.verts), 'loose_vertices')
    unseen = set(bm.faces)
    shells = 0
    while unseen:
        todo, component = [unseen.pop()], []
        while todo:
            face = todo.pop()
            component.append(face)
            for edge in face.edges:
                for other in edge.link_faces:
                    if other in unseen:
                        unseen.remove(other)
                        todo.append(other)
        shells += 1
        volume = 0.0
        for face in component:
            points = [v.co for v in face.verts]
            for i in range(1, len(points) - 1):
                volume += points[0].dot(points[i].cross(points[i + 1])) / 6.0
        check(volume > 1e-9, 'outward_normals')
    bm.free()
    check(shells == spec['expected_shells'], 'shell_count')
    return {'id': spec['id'], 'passed': not errors, 'errors': errors,
            'dimensions_m': dims, 'triangles': tris, 'shells': shells,
            'uv': 'debug_overlap_not_lightmap', 'rig': 'not_applicable_static_blockout',
            'destruction': 'not_certified'}
