"""Render seven one-second use studies as one original Blender review video."""
import json
from pathlib import Path
import bpy

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'ContentPipeline/Generated/ToolsBlockout'
REPORT=ROOT/'ContentPipeline/reports/tools_video.json'
result={'passed':False,'scope':'Blender studio preview, not gameplay capture'}
try:
    assert json.loads((ROOT/'ContentPipeline/reports/tools_blockout_validation.json').read_text())['passed']
    cfg=json.loads((ROOT/'Tools/Blender/configs/tools_blockout.json').read_text())
    bpy.ops.wm.open_mainfile(filepath=str(OUT/'Use_scanner.blend'))
    rig=bpy.data.objects['SKEL_Humanoid_A'];scene=bpy.context.scene
    samples=[]
    for spec in cfg['assets']:
        rig.animation_data.action=bpy.data.actions['AN_Human_'+spec['action']+'_Tool_Blockout']
        for f in range(1,31):
            scene.frame_set(f)
            samples.append([(pb.name,tuple(pb.rotation_quaternion),tuple(pb.location),tuple(pb.scale)) for pb in rig.pose.bones])
    action=bpy.data.actions.new('AN_Tools_Review_Only');rig.animation_data.action=action
    for f,poses in enumerate(samples,1):
        for name,rotation,location,scale in poses:
            pb=rig.pose.bones[name];pb.rotation_quaternion=rotation;pb.location=location;pb.scale=scale
            for attr in ['rotation_quaternion','location','scale']:pb.keyframe_insert(attr,frame=f)
    props=[o for o in bpy.data.objects if o.name.startswith('SM_')]
    rotor=bpy.data.objects[cfg['drill_rotor']['id']];rotor.animation_data_clear()
    scene.timeline_markers.clear()
    for obj in props:obj.hide_set(False)
    for i,spec in enumerate(cfg['assets']):
        frame=i*30+1;scene.timeline_markers.new(spec['label'],frame=frame)
        for obj in props:
            visible=obj.name==spec['id'] or (spec['kind']=='drill' and obj==rotor)
            obj.hide_render=not visible
            obj.keyframe_insert('hide_render',frame=frame)
    for f in range(1,len(samples)+1):
        rotor.rotation_euler.y=cfg['drill_rotor']['rotation_y_rad_per_second']*(f-1)/30
        rotor.keyframe_insert('rotation_euler',frame=f)
    # Visible labels show the current study without introducing game UI.
    scene.render.use_stamp=True;scene.render.use_stamp_marker=True
    scene.render.use_stamp_time=False;scene.render.use_stamp_date=False
    scene.render.use_stamp_frame=False;scene.render.use_stamp_camera=False
    scene.render.use_stamp_scene=False;scene.render.use_stamp_filename=False
    scene.render.use_stamp_render_time=False;scene.render.stamp_font_size=20
    scene.frame_start=1;scene.frame_end=len(samples);scene.frame_set(1)
    scene.render.resolution_x=640;scene.render.resolution_y=544
    scene.cycles.samples=8;scene.cycles.use_denoising=True;scene.render.use_persistent_data=True
    scene.render.image_settings.media_type='VIDEO';scene.render.image_settings.file_format='FFMPEG'
    scene.render.ffmpeg.format='MPEG4';scene.render.ffmpeg.codec='H264'
    scene.render.ffmpeg.constant_rate_factor='MEDIUM';scene.render.filepath=str(OUT/'Tools_Use_Review.mp4')
    bpy.ops.render.render(animation=True)
    clip=bpy.data.movieclips.load(str(OUT/'Tools_Use_Review.mp4'))
    assert clip.frame_duration==210 and tuple(clip.size)==(640,544)
    result.update(passed=True,frames=210,fps=30,duration_seconds=7,path='ContentPipeline/Generated/ToolsBlockout/Tools_Use_Review.mp4')
finally:
    REPORT.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print('ASTRAEON_TOOLS_VIDEO: PASS')
