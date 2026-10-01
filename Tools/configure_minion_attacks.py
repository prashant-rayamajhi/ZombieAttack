import os
import shutil
import unreal
unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)

#既存のパンチを残し、同じSkeletonの蹴りを通常敵の選択肢へ追加する。
folder = "/Game/Assets/Enemy/Animation/Minion/AnimMontage"
clip = unreal.load_asset("/Game/Assets/Enemy/Animation/Minion/AnimationSequence/Zombie_Kicking")
assert clip, "Zombie_Kicking is missing"
montage = unreal.load_asset(folder + "/AM_Kick")
if not montage:
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("source_animation", clip)
    factory.set_editor_property("target_skeleton", clip.get_editor_property("skeleton"))
    montage = unreal.AssetToolsHelpers.get_asset_tools().create_asset("AM_Kick", folder, unreal.AnimMontage, factory)
assert montage, "Kick montage creation failed"
for track in unreal.AnimationLibrary.get_animation_notify_track_names(montage):
    unreal.AnimationLibrary.remove_animation_notify_events_by_track(montage, track)
tracks = unreal.AnimationLibrary.get_animation_notify_track_names(montage)
if not tracks:
    unreal.AnimationLibrary.add_animation_notify_track(montage, "Contact")
    tracks = ["Contact"]
length = montage.get_editor_property("sequence_length")
state = unreal.AnimationLibrary.add_animation_notify_state_event(
    montage, tracks[0], 0.60, 0.40, unreal.AnimNotifyState_EnemyAttackCollision)
state.set_editor_property("m_contactBone", "RightFoot")
unreal.EditorAssetLibrary.save_loaded_asset(montage)

#クラス既定値を書き換える前に元のBPを退避する。
path = "/Game/Blueprints/Enemy/Actors/BP_Enemy"
source = os.path.join(unreal.Paths.project_content_dir(), "Blueprints/Enemy/Actors/BP_Enemy.uasset")
backup = os.path.join(unreal.Paths.project_saved_dir(), "PresentationBackup/BP_Enemy.uasset")
os.makedirs(os.path.dirname(backup), exist_ok=True)
if not os.path.exists(backup):
    shutil.copy2(source, backup)
defaults = unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(path))
defaults.set_editor_property("m_attackChoices", [unreal.load_asset(folder + "/AM_Punch"), montage])
defaults.set_editor_property("m_attackRange", 60.0)
unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
#ボスも衝撃波の広い射程と手足の接触距離を分け、届く前に空振りを始めない。
for name, distance in (("BP_MidBossChara", 80.0), ("BP_FinalBossChara", 100.0)):
    path = "/Game/Blueprints/Enemy/Actors/" + name
    source = os.path.join(unreal.Paths.project_content_dir(), "Blueprints/Enemy/Actors/" + name + ".uasset")
    backup = os.path.join(unreal.Paths.project_saved_dir(), "PresentationBackup/" + name + ".uasset")
    if not os.path.exists(backup):
        shutil.copy2(source, backup)
    defaults = unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(path))
    defaults.set_editor_property("m_attackRange", distance)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
unreal.log("Kick length: " + str(length))
