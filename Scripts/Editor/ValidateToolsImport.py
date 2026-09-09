"""Transient static tool + shared humanoid animation import; no gameplay writes."""
import hashlib
import json
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
DEST='/Game/Astraeon/ArtValidation/ToolsBlockout'


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def import_asset(path,name,kind,skeleton=None):
    options=unreal.FbxImportUI()
    types={'static':unreal.FBXImportType.FBXIT_STATIC_MESH,
           'skeletal':unreal.FBXImportType.FBXIT_SKELETAL_MESH,
           'animation':unreal.FBXImportType.FBXIT_ANIMATION}
    for key,value in {'import_mesh':kind!='animation','import_as_skeletal':kind!='static',
        'import_animations':kind=='animation','import_materials':False,'import_textures':False,
        'create_physics_asset':False,'automated_import_should_detect_type':False,
        'mesh_type_to_import':types[kind]}.items():options.set_editor_property(key,value)
    if skeleton:options.set_editor_property('skeleton',skeleton)
    data=options.get_editor_property({'static':'static_mesh_import_data','skeletal':'skeletal_mesh_import_data',
                                      'animation':'anim_sequence_import_data'}[kind])
    for key,value in {'import_uniform_scale':1.0,'convert_scene':True,'convert_scene_unit':True}.items():
        data.set_editor_property(key,value)
    if kind=='static':
        data.set_editor_property('combine_meshes',False);data.set_editor_property('auto_generate_collision',False)
    elif kind=='animation':
        data.set_editor_property('animation_length',unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        data.set_editor_property('use_default_sample_rate',True)
    task=unreal.AssetImportTask()
    for key,value in {'filename':str(path),'destination_path':DEST,'destination_name':name,
        'automated':True,'replace_existing':False,'save':False,'factory':unreal.FbxFactory(),'options':options}.items():
        task.set_editor_property(key,value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    expected={'static':unreal.StaticMesh,'skeletal':unreal.SkeletalMesh,'animation':unreal.AnimSequence}[kind]
    found=[a for a in task.get_objects() if isinstance(a,expected)]
    assert len(found)==1,'Expected exactly one '+kind+' '+name
    return found[0]


def main():
    report_path=ROOT/'ContentPipeline/reports/tools_unreal_import.json'
    report={'passed':False,'saved_packages':False,'gameplay_integrated':False,
            'engine_version':unreal.SystemLibrary.get_engine_version(),'assets':[],'animations':[]}
    try:
        source=json.loads((ROOT/'ContentPipeline/reports/tools_blockout_validation.json').read_text())
        assert source['passed']
        for key,path in [('config_sha256','Tools/Blender/configs/tools_blockout.json'),
                         ('generator_sha256','Tools/Blender/generators/tools_blockout.py'),
                         ('validator_sha256','Tools/Blender/validators/tools.py')]:
            assert source[key]==digest(ROOT/path),'Stale '+path
        for item in source['assets']+source['animation_files']:
            assert item['sha256']==digest(ROOT/item['path']),'Stale FBX '+item['id']
        assert not unreal.EditorAssetLibrary.does_directory_exist(DEST),'Existing destination: refuse overwrite'
        unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
        for item in source['assets']:
            mesh=import_asset(ROOT/item['path'],item['id'],'static')
            bounds=mesh.get_bounds();ex=bounds.box_extent;origin=bounds.origin
            actual=[2*ex.x,2*ex.y,2*ex.z]
            expected=[d*100 for d in item['dimensions_m']]
            assert all(abs(a-b)<.1 for a,b in zip(actual,expected)),'Centimetre scale mismatch'
            # convert_scene mirrors Y going from Blender's right-handed frame to Unreal's.
            center=[(a+b)*50 for a,b in zip(item['bounds_min_m'],item['bounds_max_m'])]
            center[1]=-center[1]
            measured=[origin.x,origin.y,origin.z]
            report['assets'].append({'id':item['id'],'sha256':item['sha256'],'passed':True,
                'dimensions_cm':actual,'origin_cm':measured,'expected_origin_cm':center})
            assert all(abs(a-b)<.1 for a,b in zip(measured,center)),'Grip-origin bounds mismatch'
        human=json.loads((ROOT/'ContentPipeline/reports/humanoid_blockout_validation.json').read_text())
        assert human['passed']
        body=next(i for i in human['files'] if i['id']=='SK_Human_Body_Blockout')
        assert body['sha256']==digest(ROOT/body['path'])
        mesh=import_asset(ROOT/body['path'],'SK_Human_ToolImportReference','skeletal')
        skeleton=mesh.get_editor_property('skeleton')
        for item in source['animation_files']:
            animation=import_asset(ROOT/item['path'],item['id'],'animation',skeleton)
            assert animation.get_editor_property('skeleton')==skeleton
            duration=unreal.AnimationLibrary.get_sequence_length(animation)
            assert abs(duration-1)<.002,'Duration mismatch'
            poses=[unreal.AnimPoseExtensions.get_anim_pose_at_time(animation,t,unreal.AnimPoseEvaluationOptions())
                   for t in [0,.125,.5,1]]
            names=unreal.AnimPoseExtensions.get_bone_names(poses[0])
            assert len(names)==57 and str(names[0])=='root','Skeleton mismatch'
            transforms=[unreal.AnimPoseExtensions.get_bone_pose(p,'socket_tool_r',unreal.AnimPoseSpaces.WORLD) for p in poses]
            def values(tr):return [tr.translation.x,tr.translation.y,tr.translation.z,tr.rotation.x,tr.rotation.y,tr.rotation.z,tr.rotation.w]
            points=[values(t) for t in transforms]
            assert max(abs(a-b) for a,b in zip(points[0],points[-1]))<.002,'Hold-return discontinuity'
            assert max(abs(a-b) for p in points[1:3] for a,b in zip(points[0],p))>.001,'Static tool action'
            report['animations'].append({'id':item['id'],'sha256':item['sha256'],'passed':True,'duration_seconds':duration,'bones':len(names)})
        report['passed']=True
    except Exception as exc:
        report['error']=str(exc);raise
    finally:
        report_path.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    unreal.log('ASTRAEON_TOOLS_IMPORT: PASS')


if __name__=='__main__':main()
