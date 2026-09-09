"""Render a short walk/run review video using the already validated Blender actions."""
import json
from pathlib import Path
import bpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'ContentPipeline/Generated/HumanoidBlockout'
report_path = ROOT / 'ContentPipeline/reports/humanoid_video.json'
report = {'passed': False, 'scope': 'Blender review video, not gameplay capture'}
try:
    validation = json.loads((ROOT / 'ContentPipeline/reports/humanoid_blockout_validation.json').read_text())
    assert validation['passed']
    bpy.ops.wm.open_mainfile(filepath=str(OUT / 'Humanoid_Blockout_Preview.blend'))
    scene = bpy.context.scene
    rig = bpy.data.objects['SKEL_Humanoid_A']
    samples = []
    for kind, cycles, length in [('Walk', 2, 30), ('Run', 3, 20)]:
        rig.animation_data.action = bpy.data.actions['AN_Human_' + kind + '_Blockout']
        for _ in range(cycles):
            for frame in range(1, length + 1):
                scene.frame_set(frame)
                samples.append([(pb.name, tuple(pb.rotation_quaternion), tuple(pb.location)) for pb in rig.pose.bones])
    action = bpy.data.actions.new('AN_Preview_Walk_Run_Only')
    rig.animation_data.action = action
    for frame, poses in enumerate(samples, 1):
        for name, rotation, location in poses:
            pb = rig.pose.bones[name]
            pb.rotation_quaternion, pb.location = rotation, location
            pb.keyframe_insert('rotation_quaternion', frame=frame)
            pb.keyframe_insert('location', frame=frame)
    scene.timeline_markers.clear()
    scene.timeline_markers.new('CAMINAR', frame=1)
    scene.timeline_markers.new('CORRER', frame=61)
    scene.frame_start, scene.frame_end = 1, len(samples)
    scene.frame_set(1)
    scene.render.resolution_x, scene.render.resolution_y = 512, 576
    scene.cycles.samples = 8
    scene.cycles.use_denoising = True
    scene.render.use_persistent_data = True
    scene.render.image_settings.media_type = 'VIDEO'
    scene.render.image_settings.file_format = 'FFMPEG'
    scene.render.ffmpeg.format = 'MPEG4'
    scene.render.ffmpeg.codec = 'H264'
    scene.render.ffmpeg.constant_rate_factor = 'MEDIUM'
    scene.render.filepath = str(OUT / 'Humanoid_Walk_Run.mp4')
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type == 'VIEW_3D':
                area.spaces.active.region_3d.view_perspective = 'CAMERA'
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'Humanoid_Animation_Review.blend'))
    bpy.ops.render.render(animation=True)
    assert (OUT / 'Humanoid_Walk_Run.mp4').stat().st_size > 1000
    report.update(passed=True, frames=len(samples), fps=scene.render.fps,
                  duration_seconds=len(samples)/scene.render.fps,
                  path='ContentPipeline/Generated/HumanoidBlockout/Humanoid_Walk_Run.mp4')
finally:
    report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print('ASTRAEON_HUMANOID_VIDEO: PASS')
