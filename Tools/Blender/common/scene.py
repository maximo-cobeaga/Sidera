"""Reusable metric blockout primitives. Source geometry is authored in metres."""
import hashlib
import json

import bpy
import bmesh


def reset_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1.0
    return scene


def material(name, color):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1.0)
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value = (*color, 1.0)
    shader.inputs['Roughness'].default_value = 0.65
    return mat


def create_asset(spec):
    vertices, faces = [], []
    if spec.get('profile_xz'):
        # Extruded U profile: one closed shell, including triangulated concave caps.
        profile = spec['profile_xz']
        n = len(profile)
        depth = spec['dimensions_m'][1]
        vertices = [(x, y, z) for y in (-depth / 2, depth / 2) for x, z in profile]
        faces = [tuple(range(n - 1, -1, -1)), tuple(range(n, 2 * n))]
        faces += [(i, (i + 1) % n, (i + 1) % n + n, i + n) for i in range(n)]
    else:
        for part in spec['boxes']:
            center, dims = part
            offset = len(vertices)
            vertices += [tuple(center[i] + signs[i] * dims[i] / 2 for i in range(3))
                         for signs in ((-1,-1,-1), (1,-1,-1), (1,1,-1), (-1,1,-1),
                                       (-1,-1,1), (1,-1,1), (1,1,1), (-1,1,1))]
            faces += [tuple(offset + j for j in face) for face in
                      ((0,3,2,1), (4,5,6,7), (0,1,5,4), (1,2,6,5), (2,3,7,6), (3,0,4,7))]
    mesh = bpy.data.meshes.new(spec['id'] + '_Geometry')
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(mesh)
    bm.free()
    uv = mesh.uv_layers.new(name='UV0_Debug')
    # Per-face debug UVs; intentionally overlapping, not a lightmap channel.
    for poly in mesh.polygons:
        drop = max(range(3), key=lambda i: abs(poly.normal[i]))
        axes = [i for i in range(3) if i != drop]
        for loop in poly.loop_indices:
            co = mesh.vertices[mesh.loops[loop].vertex_index].co
            uv.data[loop].uv = (co[axes[0]], co[axes[1]])
    obj = bpy.data.objects.new(spec['id'], mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(material(spec['material'], spec['color']))
    obj['quality_tier'] = 'blockout'
    obj['generator_version'] = 1
    return obj


def signature(obj):
    """Semantic determinism, independent of FBX timestamps and Blender file metadata."""
    data = {'vertices': [[round(v, 6) for v in p.co] for p in obj.data.vertices],
            'faces': [list(p.vertices) for p in obj.data.polygons],
            'materials': [m.name for m in obj.data.materials]}
    return hashlib.sha256(json.dumps(data, sort_keys=True).encode()).hexdigest()
