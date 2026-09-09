"""Seven original handheld props, separate drill rotor, shared human use actions.

Metres, +Z up/-Y forward, tool origin at main grip centre. Generated files only.
"""
import hashlib
import json
import math
from pathlib import Path
import sys

import bpy
import bmesh
from mathutils import Vector, Matrix, Quaternion

TOOL_ROOT = Path(__file__).resolve().parents[1]
ROOT = TOOL_ROOT.parents[1]
sys.path.insert(0, str(TOOL_ROOT))
from common.scene import material, reset_scene
from exporters.fbx import export_asset
from validators.tools import validate, signature, negative_controls
from generators import humanoid_blockout as human

CONFIG_PATH = TOOL_ROOT / 'configs/tools_blockout.json'
CFG = json.loads(CONFIG_PATH.read_text(encoding='utf-8'))
OUT = ROOT / 'ContentPipeline/Generated/ToolsBlockout'
REPORT = ROOT / 'ContentPipeline/reports/tools_blockout_validation.json'
PARTS = []


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def finish_part(obj, mat):
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    obj.data.materials.append(material('M_Tool_' + mat, CFG['palette'][mat]))
    PARTS.append(obj)


def box(center, size, mat, bevel=.008):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj = bpy.context.object
    obj.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    mod = obj.modifiers.new('Machined bevel', 'BEVEL')
    mod.width, mod.segments = min(bevel, min(size)/3), 2
    bpy.ops.object.modifier_apply(modifier=mod.name)
    finish_part(obj, mat)


def cylinder(center, radius, depth, mat, axis='Z', radius_tip=None, vertices=20):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius,
        radius2=radius if radius_tip is None else radius_tip, depth=depth, location=center)
    obj = bpy.context.object
    if axis == 'Y':
        obj.rotation_euler.x = math.pi/2
    elif axis == 'X':
        obj.rotation_euler.y = math.pi/2
    finish_part(obj, mat)


def ring(center, major, minor, mat, axis='Y'):
    bpy.ops.mesh.primitive_torus_add(major_segments=32, minor_segments=8,
        major_radius=major, minor_radius=minor, location=center)
    obj = bpy.context.object
    if axis == 'Y':
        obj.rotation_euler.x = math.pi/2
    finish_part(obj, mat)


def main_grip():
    cylinder((0,0,0), CFG['grip_radius_m'], CFG['grip_length_m'], 'Grip')
    for z in [-.044,-.022,0,.022,.044]:
        ring((0,0,z), .017, .0018, 'Grip', axis='Z')
    cylinder((0,0,-.072), .023, .014, 'Metal')


def geometry(kind):
    PARTS.clear()
    if kind in ('scanner','cutter','drill','hammer','maul'):
        main_grip()
    if kind == 'scanner':
        box((0,-.02,.117), (.12,.15,.105), 'Shell', .017)
        box((0,.06,.124), (.095,.018,.067), 'Grip')
        box((0,.071,.129), (.075,.009,.040), 'Cyan', .003)
        for x in [-.032,.032]:
            cylinder((x,-.109,.134), .026, .045, 'Metal', 'Y')
            cylinder((x,-.133,.134), .019, .007, 'Cyan', 'Y')
        for x in [-.025,0,.025]:
            cylinder((x,.074,.092), .005, .005, 'Metal', 'Y', vertices=12)
    elif kind == 'cutter':
        box((0,-.075,.116), (.087,.27,.077), 'Shell', .014)
        box((0,.045,.11), (.092,.04,.087), 'Grip')
        for x in [-.03,.03]:
            box((x,-.257,.117), (.022,.11,.037), 'Metal')
            box((x,-.308,.117), (.023,.014,.042), 'Ochre', .003)
        cylinder((0,-.206,.116), .016, .025, 'Ochre', 'Y')
        box((0,-.052,.163), (.04,.085,.018), 'Metal')
        for y in [-.02,-.045,-.07,-.095]:
            box((.045,y,.12), (.006,.009,.042), 'Grip', .002)
    elif kind == 'drill':
        cylinder((0,-.08,.14), .063, .22, 'Shell', 'Y')
        cylinder((0,.043,.14), .055, .026, 'Grip', 'Y')
        cylinder((0,-.211,.14), .045, .042, 'Metal', 'Y')
        ring((0,-.205,.14), .047,.007,'Ochre')
        box((0,.031,-.102), (.095,.10,.054), 'Grip')
        box((0,-.08,.209), (.08,.065,.015), 'Ochre', .003)
        # Supporting hand rail, kept away from the axial rotating bit.
        cylinder((.09,-.095,.14), .013,.10,'Metal','X')
        cylinder((.137,-.095,.101), .018,.105,'Grip')
    elif kind in ('hammer','maul'):
        large = kind == 'maul'
        length = .36 if large else .22
        cylinder((0,0,.09 + length/2), .013 if large else .011, length, 'Metal')
        top = .09+length
        box((0,0,top), (.245 if large else .16,.095 if large else .055,.095 if large else .060), 'Shell', .012)
        for x in ([-.126,.126] if large else [-.084,.084]):
            box((x,0,top), (.02,.105 if large else .06,.105 if large else .066), 'Metal')
        box((0,-(.049 if large else .029),top), (.07,.007,.067 if large else .036), 'Ochre', .002)
    elif kind == 'ration':
        box((0,-.017,.027), (.10,.045,.16), 'Pouch', .014)
        for z in [-.052,.106]:
            box((0,-.017,z), (.104,.036,.009), 'Metal', .003)
        box((0,.008,.033), (.067,.003,.067), 'Cyan', .001)
        for x in [-.03,-.01,.01,.03]:
            box((x,.011,.033), (.009,.004,.012), 'Pouch', .001)
        cylinder((.028,-.017,.12), .01,.025,'Metal')
    elif kind == 'resonator':
        cylinder((0,0,0), .021,.12,'Grip')
        box((0,-.02,.10), (.10,.085,.056), 'Shell')
        ring((0,-.02,.192), .074,.012,'Metal')
        cylinder((0,-.02,.192), .027,.06,'Cyan','Y')
        for angle in [0,math.tau/3,math.tau*2/3]:
            x,z = .057*math.sin(angle), .192+.057*math.cos(angle)
            box((x,-.02,z), (.016,.034,.02), 'Cyan', .003)
        for x in [-.043,.043]:
            cylinder((x,-.02,.14), .009,.07,'Shell')
    elif kind == 'rotor':
        cylinder((0,-.055,0), .015,.14,'Metal','Y',radius_tip=.009)
        # Solid helical cutting ribs; capped swept tubes, no non-manifold ribbon.
        for phase in [0, math.pi]:
            coords = [( .022*math.cos(phase+t*math.tau*2), -.015-.105*t,
                       .022*math.sin(phase+t*math.tau*2)) for t in [i/40 for i in range(41)]]
            verts,faces = [],[]
            for x,y,z in coords:
                for j in range(6):
                    a=math.tau*j/6
                    verts.append((x+.006*math.cos(a),y,z+.006*math.sin(a)))
            for i in range(40):
                for j in range(6):
                    a=i*6+j;b=i*6+(j+1)%6
                    faces.append((a,b,b+6,a+6))
            faces += [tuple(reversed(range(6))),tuple(range(240,246))]
            mesh=bpy.data.meshes.new('HelicalCuttingRib')
            mesh.from_pydata(verts,[],faces)
            obj=bpy.data.objects.new('HelicalCuttingRib',mesh)
            bpy.context.collection.objects.link(obj)
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            finish_part(obj,'Metal')
        cylinder((0,-.126,0), .029,.018,'Ochre','Y',radius_tip=.016)
    else:
        raise ValueError(kind)


def make_asset(spec):
    geometry(spec['kind'])
    # Build a canonical mesh rather than joining bpy objects with variable face order.
    verts, faces, indices, materials = [], [], [], []
    for part in PARTS:
        offset=len(verts)
        verts.extend([tuple(v.co) for v in part.data.vertices])
        mat=part.data.materials[0]
        if mat not in materials:
            materials.append(mat)
        polys=[]
        for p in part.data.polygons:
            f=tuple(p.vertices);start=f.index(min(f));polys.append(f[start:]+f[:start])
        for f in sorted(polys):
            faces.append(tuple(offset+i for i in f));indices.append(materials.index(mat))
        bpy.data.objects.remove(part,do_unlink=True)
    mesh=bpy.data.meshes.new(spec['id']+'_Geometry')
    mesh.from_pydata(verts,[],faces)
    for mat in materials:mesh.materials.append(mat)
    for p,i in zip(mesh.polygons,indices):p.material_index=i
    bm=bmesh.new();bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bmesh.ops.triangulate(bm,faces=[f for f in bm.faces if len(f.verts)>4])
    bm.to_mesh(mesh);bm.free()
    obj=bpy.data.objects.new(spec['id'],mesh);bpy.context.collection.objects.link(obj)
    obj['quality']='Q1 original'
    obj['item_id']=spec.get('item_id') or ('implicit_scanner' if spec['kind']=='scanner' else 'component')
    obj['pivot']='grip centre' if spec['kind']!='rotor' else 'drill bearing centre'
    bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT')
    bpy.context.view_layer.update()
    return obj


def arm_matrix(head,tail):
    q=(tail-head).to_track_quat('Y','Z')
    return Matrix.LocRotScale(head,q,Vector((1,1,1)))


def grip_binding(rig):
    hand=rig.data.bones['hand_r'].matrix_local
    direction=hand.to_3x3().col[1]
    spread=hand.to_3x3().col[0]
    z=-spread;y=-direction;x=y.cross(z)
    world=Matrix((x,y,z)).transposed().to_4x4()
    world.translation=hand.translation+direction*.105+Vector((0,-.026,0))
    return rig.data.bones['socket_tool_r'].matrix_local.inverted() @ world


def set_use_pose(rig,binding,kind,t):
    human.clear_pose(rig)
    env=math.sin(math.pi*t)**2
    position=Vector((-.20,-.48,1.32))
    rotation=Quaternion((1,0,0),math.radians(-8))
    if kind=='Scan':
        rotation=Quaternion((0,0,1),math.radians(12*math.sin(math.tau*t))) @ rotation
    elif kind=='Pulse':
        recoil=math.sin(math.pi*min(1,t*4))**2 if t<.25 else 0
        position.y+=.035*recoil;rotation=Quaternion((1,0,0),math.radians(-8-12*recoil))
    elif kind=='Drill':
        position.y-=.035*env
        position.x+=.0015*math.sin(math.tau*8*t)*env
    elif kind in ('Hammer','Maul'):
        position.z+=.10*env
        rotation=Quaternion((1,0,0),math.radians(-8+85*math.sin(math.tau*t)*env))
    elif kind=='Consume':
        position=position.lerp(Vector((-.07,-.23,1.51)),env)
        rotation=Quaternion((1,0,0),math.radians(-8-20*env))
    elif kind=='Present':
        position.z+=.08*env;position.y-=.025*env
    desired_tool=Matrix.LocRotScale(position,rotation,Vector((1,1,1)))
    socket_rest=rig.data.bones['socket_tool_r'].matrix_local
    hand_rest=rig.data.bones['hand_r'].matrix_local
    desired_hand=desired_tool @ binding.inverted() @ socket_rest.inverted() @ hand_rest
    shoulder=rig.data.bones['upperarm_r'].head_local.copy()
    target=desired_hand.translation
    upper=rig.data.bones['upperarm_r'].length;lower=rig.data.bones['lowerarm_r'].length
    delta=target-shoulder;distance=delta.length
    assert abs(upper-lower)+.005<distance<upper+lower-.002, 'Unreachable grip target'
    axis=delta.normalized();pole=Vector((-.6,.0,.85))-shoulder
    perpendicular=(pole-axis*axis.dot(pole)).normalized()
    along=(upper*upper-lower*lower+distance*distance)/(2*distance)
    elbow=shoulder+axis*along+perpendicular*math.sqrt(max(0,upper*upper-along*along))
    rig.pose.bones['upperarm_r'].matrix=arm_matrix(shoulder,elbow)
    bpy.context.view_layer.update()
    rig.pose.bones['lowerarm_r'].matrix=arm_matrix(elbow,target)
    bpy.context.view_layer.update()
    rig.pose.bones['hand_r'].matrix=desired_hand
    for finger in ['thumb','index','middle','ring','pinky']:
        for i,angle in enumerate([45,62,42],1):
            if finger=='thumb':angle*=.65
            if kind=='Consume':angle*=.65
            rig.pose.bones[f'{finger}_{i:02}_r'].rotation_quaternion=Quaternion((1,0,0),math.radians(angle))
    bpy.context.view_layer.update()
    return desired_tool


def create_actions(rig,binding,hands):
    actions={};results=[]
    for spec in CFG['assets']:
        kind=spec['action'];action=bpy.data.actions.new('AN_Human_'+kind+'_Tool_Blockout')
        action.use_fake_user=True;rig.animation_data.action=action
        matrices=[]
        for frame in range(1,32):
            bpy.context.scene.frame_set(frame)
            desired=set_use_pose(rig,binding,kind,(frame-1)/30)
            actual=rig.pose.bones['socket_tool_r'].matrix @ binding
            error=max(abs(actual[i][j]-desired[i][j]) for i in range(4) for j in range(4))
            assert error<1e-4, 'Grip does not follow target'
            assert rig.pose.bones['root'].location.length<1e-6
            evaluated=hands.evaluated_get(bpy.context.evaluated_depsgraph_get())
            skin=evaluated.to_mesh()
            try:
                assert all(math.isfinite(c) for v in skin.vertices for c in v.co)
                assert all(v.co.length<3 for v in skin.vertices),'Exploding hand skin'
            finally:
                evaluated.to_mesh_clear()
            matrices.append(tuple(round(c,5) for row in actual for c in row))
            for pb in rig.pose.bones:
                pb.keyframe_insert('rotation_quaternion',frame=frame,group=pb.name)
                pb.keyframe_insert('location',frame=frame,group=pb.name)
                pb.keyframe_insert('scale',frame=frame,group=pb.name)
        assert len(set(matrices))>3,'Static action'
        assert matrices[0]==matrices[-1],'Action does not return to hold'
        actions[kind]=action
        results.append({'id':action.name,'passed':True,'frames':31,'root_motion':False,'target_and_return_validated':True})
    return actions,results


def export_actions(rig,actions):
    results=[]
    bpy.ops.object.select_all(action='DESELECT');rig.select_set(True)
    bpy.context.view_layer.objects.active=rig
    scene=bpy.context.scene;scene.frame_start=1;scene.frame_end=31;scene.render.fps=30
    original_name=rig.name;rig.name='Armature'
    try:
        for action in actions.values():
            rig.animation_data.action=action;scene.frame_set(1)
            path=OUT/(action.name+'.fbx')
            result=bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,
                object_types={'ARMATURE'},global_scale=1,apply_unit_scale=True,
                apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',
                add_leaf_bones=False,use_armature_deform_only=False,bake_anim=True,
                bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0)
            assert 'FINISHED' in result
            results.append({'id':action.name,'path':path.relative_to(ROOT).as_posix(),'sha256':digest(path)})
    finally:
        rig.name=original_name
    return results


def camera_at(location,target,scale):
    bpy.ops.object.camera_add(location=location);cam=bpy.context.object
    cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.type='ORTHO';cam.data.ortho_scale=scale;bpy.context.scene.camera=cam
    return cam


def lights():
    scene=bpy.context.scene;scene.world.color=(.12,.12,.12)
    scene.render.engine='CYCLES';scene.cycles.samples=20;scene.cycles.device='CPU'
    scene.render.resolution_x=1400;scene.render.resolution_y=900;scene.render.resolution_percentage=100
    for position,power,size in [((1,-3,5),700,4),((-3,1,4),550,3)]:
        bpy.ops.object.light_add(type='AREA',location=position);lamp=bpy.context.object
        lamp.data.energy=power;lamp.data.shape='DISK';lamp.data.size=size
        lamp.rotation_euler=(-lamp.location).to_track_quat('-Z','Y').to_euler()


def sheet(objects):
    for i,obj in enumerate(objects[:7]):
        obj.location=((i%4-1.5)*.65,(i//4)*.85, .11)
    rotor=objects[-1];rotor.location=objects[2].location+Vector(CFG['drill_rotor']['offset_m'])
    bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.01))
    bpy.context.object.data.materials.append(material('M_Tool_Studio',(.035,.052,.07)))
    for i,spec in enumerate(CFG['assets']):
        bpy.ops.object.text_add(location=((i%4-1.5)*.65-.22,(i//4)*.85-.34,.005))
        text=bpy.context.object;text.data.body=spec['label'];text.data.size=.065
        text.data.materials.append(material('M_Tool_Label',(.65,.77,.80)))
    camera_at((1.6,-3.2,3.6),(0,.4,.08),3.4);lights()
    scene=bpy.context.scene;scene.render.filepath=str(OUT/'Tools_Overview.png')
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Tools_Overview.blend'))
    bpy.ops.render.render(write_still=True)


def load_tools():
    with bpy.data.libraries.load(str(OUT/'Tools_Source.blend'),link=False) as (source,target):
        target.objects=[n for n in source.objects if n.startswith('SM_')]
    for obj in target.objects:bpy.context.collection.objects.link(obj)
    return {o.name:o for o in target.objects}


def main():
    OUT.mkdir(parents=True,exist_ok=True);REPORT.parent.mkdir(parents=True,exist_ok=True)
    report={'passed':False,'generator_version':CFG['generator_version'],'blender_version':bpy.app.version_string,
            'config_sha256':digest(CONFIG_PATH),'generator_sha256':digest(Path(__file__)),
            'validator_sha256':digest(TOOL_ROOT/'validators/tools.py'),'gameplay_integrated':False}
    try:
        reset_scene();specs=CFG['assets']+[{'id':CFG['drill_rotor']['id'],'kind':'rotor'}];objects=[];assets=[]
        for spec in specs:
            obj=make_asset(spec);result=validate(obj,CFG['triangle_budget_per_mesh'])
            first=signature(obj);other=make_asset(spec)
            assert signature(other)==first,'Non-deterministic geometry'
            bpy.data.objects.remove(other,do_unlink=True)
            obj.name=spec['id'];result['determinism_passed']=True
            path=export_asset(obj,OUT)
            result.update(path=path.relative_to(ROOT).as_posix(),sha256=digest(path),item_id=spec.get('item_id'))
            objects.append(obj);assets.append(result)
        report['negative_controls']=negative_controls(objects[0],CFG['triangle_budget_per_mesh'])
        report['assets']=assets
        bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Tools_Source.blend'))
        report['roundtrip']=[]
        for asset in assets:
            reset_scene();bpy.ops.import_scene.fbx(filepath=str(ROOT/asset['path']))
            meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert len(meshes)==1
            bpy.context.view_layer.update();actual=list(meshes[0].dimensions)
            assert all(abs(a-b)<1e-4 for a,b in zip(actual,asset['dimensions_m']))
            assert meshes[0].location.length<1e-5
            report['roundtrip'].append({'id':asset['id'],'passed':True})
        bpy.ops.wm.open_mainfile(filepath=str(OUT/'Tools_Source.blend'))
        objects=[bpy.data.objects[s['id']] for s in specs];sheet(objects)
        human_source=ROOT/'ContentPipeline/Generated/HumanoidBlockout/Humanoid_Blockout_Source.blend'
        evidence=json.loads((ROOT/'ContentPipeline/reports/humanoid_blockout_validation.json').read_text())
        assert evidence['passed'];report['humanoid_source_sha256']=digest(human_source)
        bpy.ops.wm.open_mainfile(filepath=str(human_source))
        rig=bpy.data.objects['SKEL_Humanoid_A'];rig.animation_data.action=None;human.clear_pose(rig)
        assert len(rig.data.bones)==57
        binding=grip_binding(rig)
        report['attachment']={'bone':'socket_tool_r','matrix_local_blender_m':[list(r) for r in binding],
                              'drill_rotor':CFG['drill_rotor']}
        actions,report['animations']=create_actions(rig,binding,bpy.data.objects['SK_Human_HandsFP_Blockout'])
        report['animation_files']=export_actions(rig,actions)
        tools=load_tools();body=bpy.data.objects['SK_Human_Body_Blockout'];body.hide_render=True
        hands=bpy.data.objects['SK_Human_HandsFP_Blockout'];hands.hide_set(False);hands.hide_render=False
        # Isolate the equipped hand in study scenes; original two-hand source is untouched.
        group=hands.vertex_groups.new(name='Preview_RightHandOnly')
        group.add([v.index for v in hands.data.vertices if v.co.x<0],1.0,'REPLACE')
        mask=hands.modifiers.new('Study isolate right hand','MASK');mask.vertex_group=group.name
        bpy.ops.object.empty_add();anchor=bpy.context.object;anchor.name='ToolGrip_FollowsSocket'
        constraint=anchor.constraints.new('COPY_TRANSFORMS');constraint.target=rig;constraint.subtarget='socket_tool_r'
        for spec in CFG['assets']:
            obj=tools[spec['id']];obj.parent=anchor;obj.matrix_local=binding
        rotor=tools[CFG['drill_rotor']['id']];rotor.parent=tools[CFG['assets'][2]['id']]
        rotor.location=CFG['drill_rotor']['offset_m']
        rotor.rotation_mode='XYZ'
        for frame in range(1,32):
            rotor.rotation_euler.y=CFG['drill_rotor']['rotation_y_rad_per_second']*(frame-1)/30
            rotor.keyframe_insert('rotation_euler',frame=frame)
        scene=bpy.context.scene;scene.frame_start=1;scene.frame_end=31;scene.render.fps=30
        cam=camera_at((-.85,-1.8,1.85),(-.20,-.45,1.43),1.15);lights()
        scene.render.resolution_x=1000;scene.render.resolution_y=850
        for spec in CFG['assets']:
            rig.animation_data.action=actions[spec['action']]
            for key,obj in tools.items():
                obj.hide_render=(key!=spec['id'] and not (spec['kind']=='drill' and key==rotor.name))
                obj.hide_set(obj.hide_render)
            scene.frame_set(1);scene.render.filepath=str(OUT/('Grip_'+spec['kind']+'.png'))
            for screen in bpy.data.screens:
                for area in screen.areas:
                    if area.type=='VIEW_3D':area.spaces.active.region_3d.view_perspective='CAMERA'
            bpy.ops.wm.save_as_mainfile(filepath=str(OUT/('Use_'+spec['kind']+'.blend')))
            bpy.ops.render.render(write_still=True)
        # First-person study: camera height matches the documented 1.60 m eye level.
        # This is a Blender framing check, not the current C++ camera implementation.
        cam.location=(0,.03,1.6)
        cam.rotation_euler=(Vector((0,-1.5,1.36))-cam.location).to_track_quat('-Z','Y').to_euler()
        cam.data.type='PERSP';cam.data.lens=20
        scene.render.resolution_x=1280;scene.render.resolution_y=720
        for spec in CFG['assets'][:3]:
            rig.animation_data.action=actions[spec['action']]
            for key,obj in tools.items():
                obj.hide_render=(key!=spec['id'] and not (spec['kind']=='drill' and key==rotor.name))
                obj.hide_set(obj.hide_render)
            scene.frame_set(1);scene.render.filepath=str(OUT/('FP_'+spec['kind']+'.png'))
            bpy.ops.render.render(write_still=True)
        report['animation_roundtrip']=[]
        for asset in report['animation_files']:
            for obj in bpy.context.scene.objects:obj.hide_set(False)
            reset_scene();bpy.ops.import_scene.fbx(filepath=str(ROOT/asset['path']))
            rigs=[o for o in bpy.context.scene.objects if o.type=='ARMATURE']
            assert len(rigs)==1 and len(rigs[0].data.bones)==57,'Roundtrip rig count '+asset['id']
            imported=rigs[0];assert imported.animation_data and imported.animation_data.action
            action=imported.animation_data.action
            assert abs(action.frame_range[1]-action.frame_range[0]-30)<.01,('Roundtrip duration',asset['id'],list(action.frame_range))
            poses=[]
            start=int(round(action.frame_range[0]))
            for offset in [0,4,15,30]:
                f=start+offset
                scene.frame_set(f)
                poses.append(tuple(round(c,5) for row in imported.pose.bones['socket_tool_r'].matrix for c in row))
            assert len(set(poses))>1,'Roundtrip static '+asset['id']
            assert max(abs(a-b) for a,b in zip(poses[0],poses[-1]))<1e-4,'Roundtrip return '+asset['id']
            report['animation_roundtrip'].append({'id':asset['id'],'passed':True})
        report['passed']=True
    except Exception as exc:
        report['error']=str(exc);raise
    finally:
        REPORT.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('ASTRAEON_TOOLS: PASS')


if __name__=='__main__':main()
