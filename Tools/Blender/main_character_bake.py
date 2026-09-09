"""Transfer skinning and bake independent Character/Suit maps from original art."""
import json
from pathlib import Path
import time
import traceback
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ROOT = Path(r'C:/Users/MAXIMO/Desktop/Astraeon')
ASSET = ROOT / 'graphics/characters/main_player'
REPORT = ASSET / 'docs/bake_progress.json'


def activate(obj):
    if bpy.context.object and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    obj.hide_viewport = False
    obj.hide_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def barycentric(p, a, b, c):
    ab, ac, ap = b-a, c-a, p-a
    d00, d01, d11 = ab.dot(ab), ab.dot(ac), ac.dot(ac)
    denom = d00*d11-d01*d01
    if abs(denom) < 1e-20:
        return (1,0,0)
    v = (d11*ap.dot(ab)-d01*ap.dot(ac))/denom
    w = (d00*ap.dot(ac)-d01*ap.dot(ab))/denom
    weights = [max(0,1-v-w),max(0,v),max(0,w)]
    return [v/sum(weights) for v in weights]


def prepare():
    old = bpy.data.objects['SK_Astraeon_Player']
    obj = bpy.data.objects['WORK_Player_QuadRetopo']
    assert len(obj.data.vertices) < 41000 and not obj.data.uv_layers
    source_vertices = [v.co.copy() for v in old.data.vertices]
    old.data.calc_loop_triangles()
    triangles = [tuple(t.vertices) for t in old.data.loop_triangles]
    tree = BVHTree.FromPolygons(source_vertices, triangles, all_triangles=True)
    old_weights = [{old.vertex_groups[g.group].name:g.weight for g in v.groups}
                   for v in old.data.vertices]
    # Restore exact authored ground and height after solver drift (<1 mm).
    zmin=min(v.co.z for v in obj.data.vertices)
    zmax=max(v.co.z for v in obj.data.vertices)
    for v in obj.data.vertices:
        v.co.z=(v.co.z-zmin)*1.83/(zmax-zmin)
    obj.vertex_groups.clear()
    groups={g.name:obj.vertex_groups.new(name=g.name) for g in old.vertex_groups}
    weights=[]
    max_distance=0
    for vert in obj.data.vertices:
        hit, normal, index, distance = tree.find_nearest(vert.co)
        max_distance=max(max_distance,distance)
        tri=triangles[index]
        bary=barycentric(hit, *(source_vertices[i] for i in tri))
        weight={}
        for i,factor in zip(tri,bary):
            for name,value in old_weights[i].items():
                weight[name]=weight.get(name,0)+value*factor
        weight=dict(sorted(weight.items(), key=lambda p:p[1], reverse=True)[:4])
        total=sum(weight.values())
        assert total>0
        weight={k:v/total for k,v in weight.items() if v/total>.00001}
        total=sum(weight.values());weight={k:v/total for k,v in weight.items()}
        weights.append(weight)
        for name,value in weight.items():
            groups[name].add([vert.index],value,'REPLACE')
    obj.data.materials.clear()
    for region in ('Character','Suit'):
        mat=bpy.data.materials.get('MAT_Player_'+region) or bpy.data.materials.new('MAT_Player_'+region)
        mat.use_nodes=True
        obj.data.materials.append(mat)
    for face in obj.data.polygons:
        head=sum(weights[i].get('head',0) for i in face.vertices)/len(face.vertices)
        face.material_index=0 if head>.5 else 1
    activate(obj)
    obj.data.uv_layers.new(name='UVMap')
    bpy.context.tool_settings.mesh_select_mode=(False,False,True)
    for index in (0,1):
        for face in obj.data.polygons:
            face.select=face.material_index==index
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.uv.smart_project(angle_limit=1.04719755,island_margin=.012,area_weight=.3,
                                 correct_aspect=True,scale_to_bounds=False)
        bpy.ops.object.mode_set(mode='OBJECT')
    old.name='ARCHIVE_Player_233k'
    old.hide_render=True;old.hide_set(True)
    obj.name='SK_Astraeon_Player'
    obj.parent=bpy.data.objects['SKEL_Astraeon_Player']
    mod=obj.modifiers.new('ARM_Skin','ARMATURE');mod.object=obj.parent
    obj.hide_render=False
    report=dict(status='PREPARED',triangles=65284,vertices=len(obj.data.vertices),
                transfer_max_distance_m=max_distance,
                material_faces={m.name:sum(p.material_index==i for p in obj.data.polygons)
                                for i,m in enumerate(obj.data.materials)},
                unweighted_vertices=sum(not v.groups for v in obj.data.vertices))
    (ASSET/'docs/skin_transfer.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


def bake_body():
    start=time.time()
    report={'status':'BAKING','completed':[]}
    REPORT.write_text(json.dumps(report,indent=2))
    scene=bpy.context.scene
    engine=scene.render.engine
    src=bpy.data.objects['SRC_Player_MESH_01']
    obj=bpy.data.objects['SK_Astraeon_Player']
    old_mats=list(src.data.materials)
    original_visibility=(src.hide_viewport,src.hide_render,src.hide_get())
    mod=obj.modifiers.get('ARM_Skin')
    mod.show_viewport=False;mod.show_render=False
    try:
        scene.render.engine='CYCLES'
        scene.cycles.samples=16
        scene.cycles.device='CPU'
        source_material=old_mats[0].copy()
        source_material.name='BAKE_Source_Player_Temporary'
        src.data.materials.clear();src.data.materials.append(source_material)
        nt=source_material.node_tree
        output=next(n for n in nt.nodes if n.type=='OUTPUT_MATERIAL')
        original_surface=output.inputs['Surface'].links[0].from_socket
        emission=nt.nodes.new('ShaderNodeEmission')
        orm=next(n for n in nt.nodes if n.type=='TEX_IMAGE' and n.image and n.image.name.startswith('ORM'))
        maps={}
        for kind in ('Color','NormalGL','ORM','AO'):
            targets=[]
            for region,mat in zip(('Character','Suit'),obj.data.materials):
                name=f'T_Player_{region}_{kind}'
                image=bpy.data.images.get(name) or bpy.data.images.new(name,width=2048,height=2048,alpha=False)
                image.colorspace_settings.name='sRGB' if kind=='Color' else 'Non-Color'
                nodes=mat.node_tree.nodes
                target=nodes.get('BAKE_TARGET') or nodes.new('ShaderNodeTexImage')
                target.name='BAKE_TARGET';target.image=image;nodes.active=target
                targets.append(image);maps[(region,kind)]=image
            activate(obj)
            src.hide_viewport=False;src.hide_render=False;src.hide_set(False);src.select_set(True)
            if kind=='ORM':
                nt.links.new(orm.outputs['Color'],emission.inputs['Color'])
                nt.links.new(emission.outputs[0],output.inputs['Surface'])
            else:
                nt.links.new(original_surface,output.inputs['Surface'])
            kwargs=dict(type={'Color':'DIFFUSE','NormalGL':'NORMAL','ORM':'EMIT','AO':'AO'}[kind],
                        use_selected_to_active=True,cage_extrusion=.006,max_ray_distance=.025,
                        margin=12,use_clear=True)
            if kind=='Color':kwargs['pass_filter']={'COLOR'}
            if kind=='NormalGL':kwargs.update(normal_space='TANGENT',normal_g='POS_Y')
            assert 'FINISHED' in bpy.ops.object.bake(**kwargs)
            for image in targets:
                image.filepath_raw=str(ASSET/'textures'/f'{image.name}.png')
                image.file_format='PNG';image.save()
                report['completed'].append(image.filepath_raw)
            REPORT.write_text(json.dumps(report,indent=2))
        for region,mat in zip(('Character','Suit'),obj.data.materials):
            nodes=mat.node_tree.nodes;links=mat.node_tree.links
            nodes.clear()
            out=nodes.new('ShaderNodeOutputMaterial');bsdf=nodes.new('ShaderNodeBsdfPrincipled')
            links.new(bsdf.outputs[0],out.inputs['Surface'])
            images={}
            for kind in ('Color','NormalGL','ORM'):
                n=nodes.new('ShaderNodeTexImage');n.image=maps[(region,kind)];n.name=kind;images[kind]=n
            links.new(images['Color'].outputs['Color'],bsdf.inputs['Base Color'])
            norm=nodes.new('ShaderNodeNormalMap');links.new(images['NormalGL'].outputs['Color'],norm.inputs['Color'])
            links.new(norm.outputs['Normal'],bsdf.inputs['Normal'])
            split=nodes.new('ShaderNodeSeparateColor');links.new(images['ORM'].outputs['Color'],split.inputs[0])
            links.new(split.outputs[1],bsdf.inputs['Roughness']);links.new(split.outputs[2],bsdf.inputs['Metallic'])
        report['status']='BAKED_AWAITING_VISUAL_QA'
    except Exception:
        report['status']='FAILED';report['error']=traceback.format_exc()
    finally:
        src.data.materials.clear()
        for mat in old_mats:src.data.materials.append(mat)
        src.hide_viewport,src.hide_render=original_visibility[:2];src.hide_set(original_visibility[2])
        scene.render.engine=engine
        mod.show_viewport=True;mod.show_render=True
        report['elapsed_s']=time.time()-start
        REPORT.write_text(json.dumps(report,indent=2)+'\n')
    return None
