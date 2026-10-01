import json
import os
import unreal

#本編マップに配置された敵と生成地点の設定を読み取り、クラス既定値との違いを調べる。
unreal.EditorLoadingAndSavingUtils.load_map("/Game/Map/GameLevel1")
report = []
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor, unreal.EnemyChara):
        mesh = actor.get_editor_property("mesh")
        item = {"actor": actor.get_actor_label(), "class": actor.get_class().get_path_name(),
                "mesh": mesh.get_editor_property("skeletal_mesh_asset").get_path_name(), "attacks": {}}
        for field in ("m_pAttackMontage", "m_pDeathMontage", "m_pLightComboMontage", "m_pPowerSlamMontage", "m_pChargeRushMontage"):
            try:
                asset = actor.get_editor_property(field)
                item["attacks"][field] = asset.get_path_name() if asset else None
            except Exception:
                pass
        report.append(item)
    elif "Spawn" in actor.get_class().get_name():
        report.append({"spawn": actor.get_actor_label(), "class": actor.get_class().get_path_name()})
with open(os.path.join(unreal.Paths.project_saved_dir(), "Tests/enemy_level_audit.json"), "w", encoding="utf-8") as stream:
    json.dump(report, stream, ensure_ascii=False, indent=2)
