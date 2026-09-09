"""Local joint-weight corrections and additive forearm/upperarm twist helpers."""
import json
from pathlib import Path
import bpy
from mathutils import Vector


def polish():
    obj=bpy.data.objects['SK_Astraeon_Player']
    rig=bpy.data.objects['SKEL_Astraeon_Player']
    assert not any('twist_01' in b.name for b in rig.data.bones), 'Already polished'
    if bpy.context.object and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    rig.hide_set(False);rig.select_set(True)
    bpy.context.view_layer.objects.active=rig
    bpy.ops.object.mode_set(mode='EDIT')
    added=[]
    for side in ('l','r'):
        for limb in ('upperarm','lowerarm'):
            original=rig.data.edit_bones[f'{limb}_{side}']
            name=f'{limb}_twist_01_{side}'
            bone=rig.data.edit_bones.new(name)
            bone.head=original.head.lerp(original.tail,.3)
            bone.tail=original.head.lerp(original.tail,.6)
            bone.roll=original.roll;bone.parent=original;bone.use_deform=True
            added.append((name,original.name))
    bpy.ops.object.mode_set(mode='OBJECT')
    for name,source in added:
        bone=rig.pose.bones[name];bone.rotation_mode='XYZ'
        curve=bone.driver_add('rotation_euler',1)
        driver=curve.driver;driver.type='SCRIPTED'
        var=driver.variables.new();var.name='source_twist';var.type='SINGLE_PROP'
        var.targets[0].id=rig
        var.targets[0].data_path=f'pose.bones["{source}"].rotation_euler[1]'
        driver.expression='-0.5*source_twist'
        bone['purpose']='Counter half local axial twist; baked into exported clips'
    weights=[{obj.vertex_groups[g.group].name:g.weight for g in v.groups} for v in obj.data.vertices]
    neighbors=[set() for _ in obj.data.vertices]
    for edge in obj.data.edges:
        a,b=edge.vertices;neighbors[a].add(b);neighbors[b].add(a)
    joints={'clavicle_l','clavicle_r','upperarm_l','upperarm_r','lowerarm_l','lowerarm_r',
            'hand_l','hand_r','pelvis','thigh_l','thigh_r'}
    # Correct the spatial transfer specifically at blended joints. Retain
    # rigid interiors; smoothing only where two significant weights meet.
    painted=set()
    for _ in range(3):
        updated=[]
        for i,weight in enumerate(weights):
            significant=[name for name,value in weight.items() if value>.12]
            if len(significant)<2 or not any(n in joints for n in significant):
                updated.append(weight);continue
            average={}
            for n in neighbors[i]:
                for name,value in weights[n].items():
                    average[name]=average.get(name,0)+value/len(neighbors[i])
            mix={name:.75*weight.get(name,0)+.25*average.get(name,0) for name in set(weight)|set(average)}
            updated.append(mix);painted.add(i)
        weights=updated
    for name,source in added:
        original=rig.data.bones[source]
        direction=original.tail_local-original.head_local
        length_sq=direction.length_squared
        for v,weight in zip(obj.data.vertices,weights):
            if source not in weight:continue
            t=max(0,min(1,(v.co-original.head_local).dot(direction)/length_sq))
            fraction=.65*(1-t)
            weight[name]=weight[source]*fraction
            weight[source]*=1-fraction
    obj.vertex_groups.clear()
    groups={name:obj.vertex_groups.new(name=name) for name in {n for w in weights for n in w}}
    for v,weight in zip(obj.data.vertices,weights):
        weight=dict(sorted(weight.items(),key=lambda p:p[1],reverse=True)[:4])
        total=sum(weight.values())
        for name,value in weight.items():
            if value>1e-6:groups[name].add([v.index],value/total,'REPLACE')
    result={'bones':len(rig.data.bones),'twist_bones':[n for n,s in added],
            'joint_vertices_corrected':len(painted),'method':'localized authored weight smoothing + axial twist distribution',
            'manual_ui_weight_paint':False,'deformation_visual_audit':'pending'}
    path=Path(r'C:/Users/MAXIMO/Desktop/Astraeon/graphics/characters/main_player/docs/skin_polish.json')
    path.write_text(json.dumps(result,indent=2)+'\n')
    return result
