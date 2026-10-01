import unreal
import statistics
unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
for name in ('Pistol_Run_Backward', 'Backwards_Rifle_Walk', 'Warrior_Idle', 'Falling_Back_Death'):
    clip = unreal.load_asset('/Game/Assets/Player/Animation/AnimSequence/' + name)
    length = clip.get_editor_property('sequence_length')
    unreal.log('MOTION ' + name + ' duration=' + str(length) + ' skeleton=' + str(clip.get_editor_property('skeleton')))
    for frame in range(13):
        pose = unreal.AnimationLibrary.get_bone_pose_for_time(clip, 'Hips', length * frame / 12.0, False)
        unreal.log('ROOT ' + name + ' ' + str(frame) + ' ' + str(pose.translation))
    if name not in ('Pistol_Run_Backward', 'Backwards_Rifle_Walk'):
        continue
    speeds = []
    for side in ('Left', 'Right'):
        poses = []
        for frame in range(121):
            time = length * frame / 120.0
            pose = unreal.AnimationLibrary.get_bone_pose_for_time(clip, 'Hips', time, False)
            for bone in (side + 'UpLeg', side + 'Leg', side + 'Foot'):
                local = unreal.AnimationLibrary.get_bone_pose_for_time(clip, bone, time, False)
                pose = unreal.MathLibrary.compose_transforms(local, pose)
            poses.append(pose.translation)
        low = min(p.z for p in poses)
        for before, after in zip(poses, poses[1:]):
            if max(before.z, after.z) < low + 7.0:
                speeds.append(abs(after.y - before.y) * 120.0 / length)
    unreal.log('STANCE SPEED ' + name + ' ' + str(statistics.median(speeds)))
