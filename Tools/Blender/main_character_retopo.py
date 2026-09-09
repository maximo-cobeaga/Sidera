"""Live-Bridge semi-automatic quad retopology, keeping authored sources intact."""
import json
from pathlib import Path
import time
import traceback
import bpy
import bmesh
from mathutils import Matrix

ROOT = Path(r'C:/Users/MAXIMO/Desktop/Astraeon')
REPORT = ROOT / 'graphics/characters/main_player/docs/retopo_progress.json'


def run(repair_volume=False, original_source=False):
    started = time.time()
    report = dict(status='RUNNING', method='QuadriFlow on welded original source',
                  target_quads=36000, seed=831)
    REPORT.write_text(json.dumps(report, indent=2))
    try:
        obj = bpy.data.objects['WORK_Player_QuadRetopo']
        if original_source:
            obj.data = bpy.data.objects['SRC_Player_MESH_01'].data.copy()
        if bpy.context.object and bpy.context.object.mode != 'OBJECT':
            bpy.ops.object.mode_set(mode='OBJECT')
        bpy.ops.object.select_all(action='DESELECT')
        obj.hide_set(False)
        obj.hide_viewport = False
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        if repair_volume:
            # The original has coincident disconnected fans which QuadriFlow's
            # internal triangulation rejects even after BMesh manifold checks.
            # Reconstruct the volume at 1.8 mm, then project/bake onto the intact
            # high-resolution source. This is not the delivery topology.
            obj.data.remesh_voxel_size = .0018
            obj.data.use_remesh_preserve_volume = True
            report['volume_repair_m'] = .0018
            assert 'FINISHED' in bpy.ops.object.voxel_remesh()
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        if original_source:
            bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=.00001)
            bmesh.ops.dissolve_degenerate(bm, dist=.0000001, edges=list(bm.edges))
        # Original source has one tiny dangling triangular flap. Select it by
        # topology (two boundary edges), never by a spatial crop.
        flaps = [f for f in bm.faces if sum(e.is_boundary for e in f.edges) == 2]
        report['removed_dangling_faces'] = len(flaps)
        assert len(flaps) <= 2, 'Unexpected source topology; refusing broad deletion'
        if flaps:
            bmesh.ops.delete(bm, geom=flaps, context='FACES')
        # Welding the UV splits can join distinct face fans at the forearm.
        # Split only disconnected fans, preserving their faces and positions.
        report['split_bowtie_vertices'] = sum(len(bmesh.utils.vert_separate(v, [])) - 1
            for v in list(bm.verts) if not v.is_manifold and v.link_faces)
        assert all(v.is_manifold for v in bm.verts)
        report['nonmanifold_before_solver'] = sum(not e.is_manifold for e in bm.edges)
        assert report['nonmanifold_before_solver'] == 0, 'Source must be manifold'
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        bm.to_mesh(obj.data)
        bm.free()
        REPORT.write_text(json.dumps(report, indent=2))
        # Blender's manifold preflight rejects edges with component distances
        # <=1e-4 in local coordinates (object_remesh.cc). Dense metre-scale
        # geometry hits that threshold despite valid manifold topology.
        # Temporarily solve in millimetres, then restore metre coordinates.
        obj.data.transform(Matrix.Scale(1000, 4))
        try:
            outcome = bpy.ops.object.quadriflow_remesh(use_mesh_symmetry=False,
                        use_preserve_sharp=True, use_preserve_boundary=False,
                        preserve_attributes=False, smooth_normals=True,
                        mode='FACES', target_faces=36000, seed=831)
        finally:
            obj.data.transform(Matrix.Scale(.001, 4))
        report['solver_coordinate_scale'] = 1000
        obj.data.calc_loop_triangles()
        report.update(operator=list(outcome), vertices=len(obj.data.vertices),
                      polygons=len(obj.data.polygons), triangles=len(obj.data.loop_triangles),
                      quads=sum(len(p.vertices) == 4 for p in obj.data.polygons))
        assert 'FINISHED' in outcome and 60000 <= report['triangles'] <= 80000
        report['status'] = 'TOPOLOGY_READY_AWAITING_BAKE_AND_DEFORMATION_AUDIT'
        obj.hide_render = True
        obj.hide_set(True)
        checkpoint = ROOT / 'Saved/BlenderRecovery/CHK_Player_quad_topology_20260908.blend'
        bpy.ops.wm.save_as_mainfile(filepath=str(checkpoint), copy=True)
        report['checkpoint'] = str(checkpoint)
    except Exception:
        report['status'] = 'FAILED'
        report['error'] = traceback.format_exc()
    report['elapsed_s'] = time.time() - started
    REPORT.write_text(json.dumps(report, indent=2) + '\n')
    return None
