"""Restore the Blender Armature unit scale dropped by standalone FBX animation import.

The existing meshes bind root at scale 100 (metres -> centimetres). The importer
removes the Armature container but writes scale 1 in animation root keys. Restore
only this verified mismatch; never scale the component, translations or other bones.
"""
import unreal


def normalize_root_scale(anim):
    skeleton = anim.get_editor_property('skeleton')
    ref = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
    expected = unreal.AnimPoseExtensions.get_bone_pose(ref, 'root', unreal.AnimPoseSpaces.LOCAL).scale3d
    options = unreal.AnimPoseEvaluationOptions()
    options.set_editor_property('evaluation_type', unreal.AnimDataEvalType.SOURCE)
    options.set_editor_property('should_retarget', False)
    model = anim.get_editor_property('data_model_interface')
    count = model.get_number_of_keys()
    duration = unreal.AnimationLibrary.get_sequence_length(anim)
    roots = []
    for i in range(count):
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, duration * i / max(count - 1, 1), options)
        roots.append(unreal.AnimPoseExtensions.get_bone_pose(pose, 'root', unreal.AnimPoseSpaces.LOCAL))
    def close(a, b):
        return max(abs(a.x-b.x), abs(a.y-b.y), abs(a.z-b.z)) < .001
    if all(close(t.scale3d, expected) for t in roots):
        return False
    assert close(expected, unreal.Vector(100, 100, 100)), 'Unexpected reference scale: ' + anim.get_path_name()
    assert all(close(t.scale3d, unreal.Vector(1, 1, 1)) for t in roots), 'Nonuniform/animated root scale: ' + anim.get_path_name()
    controller = anim.get_editor_property('controller')
    assert controller.set_bone_track_keys('root', [t.translation for t in roots],
                                          [t.rotation for t in roots], [expected] * count, False)
    return True


def validate_pose_scale(anim):
    """Check evaluated proportions at five samples, including crouch/tool gestures."""
    options = unreal.AnimPoseEvaluationOptions()
    duration = unreal.AnimationLibrary.get_sequence_length(anim)
    heights = []
    for fraction in (0, .25, .5, .75, 1):
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, duration * fraction, options)
        head = unreal.AnimPoseExtensions.get_bone_pose(pose, 'head', unreal.AnimPoseSpaces.WORLD).translation
        root = unreal.AnimPoseExtensions.get_bone_pose(pose, 'root', unreal.AnimPoseSpaces.WORLD).translation
        height = head.z-root.z
        assert 65 < height < 220, f'Animated head height {height} cm: {anim.get_path_name()}'
        heights.append(height)
    return heights
