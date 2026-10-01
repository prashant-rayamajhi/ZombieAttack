import os
import shutil
import unreal

unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)

#変更するアセットはSavedへ退避し、エディタ側の設定も元へ戻せるようにする。
backup = os.path.join(unreal.Paths.project_saved_dir(), "EnemyAnimationBackup")
def save_asset(asset):
    package = asset.get_path_name().split(".")[0]
    relative = package.removeprefix("/Game/") + ".uasset"
    source = os.path.join(unreal.Paths.project_content_dir(), relative)
    destination = os.path.join(backup, relative)
    os.makedirs(os.path.dirname(destination), exist_ok=True)
    if not os.path.exists(destination):
        shutil.copy2(source, destination)
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

#スラムは単独の範囲攻撃として選び、通常のパンチ連撃からは外す。
path = "/Game/Blueprints/Enemy/Actors/BP_FinalBossChara"
blueprint = unreal.load_asset(path)
defaults = unreal.get_default_object(unreal.EditorAssetLibrary.load_blueprint_class(path))
combo = list(defaults.get_editor_property("m_comboAttackMontages"))
filtered = [montage for montage in combo if montage and "Slam" not in montage.get_name()]
if filtered != combo:
    defaults.set_editor_property("m_comboAttackMontages", filtered)
    save_asset(blueprint)

#腰が最上位骨のモデルでは、ルート固定で腰の回転と沈み込みまで失われる。
#水平移動の制限はEnemyAnimInstanceで行い、クリップの腰の演技は残す。
paths = set(unreal.EditorAssetLibrary.list_assets("/Game/Assets/Enemy", recursive=True))
registry = unreal.AssetRegistryHelpers.get_asset_registry()
for path in list(paths):
    if isinstance(unreal.load_asset(path), unreal.AnimMontage):
        paths.update(str(p) for p in registry.get_dependencies(path.split(".")[0], unreal.AssetRegistryDependencyOptions(True, True)))
for path in paths:
    asset = unreal.load_asset(path)
    if isinstance(asset, unreal.AnimMontage) and "Rush" in asset.get_name():
        #短い突進クリップをブレンドだけで使い切らないよう、入りと終わりを短くする。
        for property_name in ("blend_in", "blend_out"):
            blend = asset.get_editor_property(property_name)
            blend.set_editor_property("blend_time", 0.08)
            asset.set_editor_property(property_name, blend)
        save_asset(asset)
    if not isinstance(asset, unreal.AnimSequence):
        continue
    if asset.get_editor_property("enable_root_motion") or asset.get_editor_property("force_root_lock"):
        asset.set_editor_property("enable_root_motion", False)
        asset.set_editor_property("force_root_lock", False)
        save_asset(asset)

#蹴りの接触通知だけは足へ付け替え、手からダメージが出ないようにする。
kick = unreal.load_asset("/Game/Assets/Enemy/Animation/MidBoss/AnimMontage/Chapa_2_Montage")
if kick:
    for event in unreal.AnimationLibrary.get_animation_notify_events(kick):
        state = event.get_editor_property("notify_state_class")
        if isinstance(state, unreal.AnimNotifyState_EnemyAttackCollision):
            state.set_editor_property("m_contactBone", "RightFoot")
    save_asset(kick)
