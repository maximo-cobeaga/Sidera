"""Static tool contract: grip-origin instead of architectural ground-origin."""
import hashlib
import json
import math
import bmesh


def signature(obj):
    # Compare topology independently of bpy polygon enumeration.
    data = {'v': [[round(c, 6) for c in v.co] for v in obj.data.vertices],
            'f': sorted((tuple(p.vertices), p.material_index) for p in obj.data.polygons),
            'm': [m.name for m in obj.data.materials]}
    return hashlib.sha256(json.dumps(data, sort_keys=True).encode()).hexdigest()


def validate(obj, budget):
    assert obj.type == 'MESH' and obj.data.vertices
    assert obj.location.length < 1e-6, 'Grip pivot displaced'
    assert all(abs(s-1) < 1e-6 for s in obj.scale), 'Unapplied scale'
    assert obj.rotation_euler.to_matrix().is_identity, 'Unapplied rotation'
    assert not obj.modifiers and obj.parent is None, 'Unexpected modifier or parent'
    assert 1 <= len(obj.data.materials) <= 4, 'Material budget'
    assert obj.data.uv_layers, 'Missing UV'
    assert all(math.isfinite(c) for v in obj.data.vertices for c in v.co)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    try:
        assert all(e.is_manifold and e.is_contiguous for e in bm.edges), 'Open or inverted shell edge'
        assert all(f.calc_area() > 1e-10 for f in bm.faces), 'Degenerate face'
        assert bm.calc_volume(signed=True) > 0, 'Inward normals'
    finally:
        bm.free()
    obj.data.calc_loop_triangles()
    tris = len(obj.data.loop_triangles)
    assert tris <= budget, 'Triangle budget'
    low = [min(v.co[i] for v in obj.data.vertices) for i in range(3)]
    high = [max(v.co[i] for v in obj.data.vertices) for i in range(3)]
    dims = [b-a for a,b in zip(low,high)]
    assert all(.005 < d < 1.5 for d in dims), 'Implausible hand-tool size'
    return {'id': obj.name, 'passed': True, 'dimensions_m': dims, 'bounds_min_m': low,
            'bounds_max_m': high, 'triangles': tris, 'materials': len(obj.data.materials),
            'geometry_sha256': signature(obj)}


def negative_controls(obj, budget):
    results = []
    def reject(name, corrupt, restore):
        corrupt()
        caught = False
        try:
            validate(obj, budget)
        except AssertionError:
            caught = True
        finally:
            restore()
        assert caught, 'Accepted invalid ' + name
        results.append({'case': name, 'rejected': True})
    reject('displaced_grip', lambda: setattr(obj, 'location', (0,0,.1)), lambda: setattr(obj, 'location', (0,0,0)))
    reject('unapplied_scale', lambda: setattr(obj, 'scale', (100,100,100)), lambda: setattr(obj, 'scale', (1,1,1)))
    mesh = obj.data
    broken = mesh.copy()
    bm = bmesh.new()
    bm.from_mesh(broken)
    bm.faces.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[bm.faces[0]], context='FACES')
    bm.to_mesh(broken)
    bm.free()
    reject('open_shell', lambda: setattr(obj, 'data', broken), lambda: setattr(obj, 'data', mesh))
    return results
