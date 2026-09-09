"""Negative controls ensure validation rejects broken source assets."""
import bpy
import bmesh
from common.scene import create_asset, signature
from validators.mesh import validate


def run(config):
    results = []
    spec = config['assets'][1]
    def case(label, mutate, expected):
        obj = create_asset(spec)
        mutate(obj)
        bpy.context.view_layer.update()
        result = validate(obj, spec)
        passed = expected in result['errors']
        results.append({'test': label, 'passed': passed, 'detected': result['errors']})
        bpy.data.objects.remove(obj, do_unlink=True)
    case('unapplied_scale', lambda o: setattr(o, 'scale', (2,1,1)), 'applied_scale')
    case('wrong_name', lambda o: setattr(o, 'name', 'Wrong'), 'object_name')
    case('wrong_pivot', lambda o: setattr(o, 'location', (1,0,0)), 'origin')
    case('missing_uv', lambda o: o.data.uv_layers.remove(o.data.uv_layers[0]), 'missing_debug_uv')
    case('missing_material', lambda o: o.data.materials.clear(), 'material_slots')
    case('modifier', lambda o: o.modifiers.new('Unexpected', 'BEVEL'), 'unsupported_modifiers')
    case('unapplied_rotation', lambda o: setattr(o, 'rotation_euler', (0,0,0.5)), 'applied_rotation')
    def resize(o):
        for vertex in o.data.vertices:
            vertex.co.x *= 2
        o.data.update()
    case('wrong_geometry_dimensions', resize, 'dimensions_m')
    def flip(o):
        bm = bmesh.new()
        bm.from_mesh(o.data)
        bmesh.ops.reverse_faces(bm, faces=list(bm.faces))
        bm.to_mesh(o.data)
        bm.free()
    case('inverted_normals', flip, 'outward_normals')
    def hole(o):
        bm = bmesh.new()
        bm.from_mesh(o.data)
        bmesh.ops.delete(bm, geom=[next(iter(bm.faces))], context='FACES')
        bm.to_mesh(o.data)
        bm.free()
    case('open_mesh', hole, 'manifold_or_winding')
    for asset in config['assets']:
        first = create_asset(asset)
        first_hash = signature(first)
        bpy.data.objects.remove(first, do_unlink=True)
        second = create_asset(asset)
        bpy.context.view_layer.update()
        results.append({'test': asset['id'] + '_repeat',
                        'passed': first_hash == signature(second) and validate(second, asset)['passed']})
        bpy.data.objects.remove(second, do_unlink=True)
    return results
