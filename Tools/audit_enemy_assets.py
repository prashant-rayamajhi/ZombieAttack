import json
import os
import unreal

report = {"enemies": [], "animations": [], "blueprints": []}
unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)

def read(obj, name):
    try:
        value = obj.get_editor_property(name)
        if isinstance(value, unreal.Object):
            return value.get_path_name()
        return str(value)
    except Exception as error:
        return "UNAVAILABLE: " + str(error)

for name in ["BP_Enemy", "BP_MidBossChara", "BP_FinalBossChara"]:
    path = "/Game/Blueprints/Enemy/Actors/" + name
    cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
    obj = unreal.get_default_object(cls)
    mesh = obj.get_editor_property("mesh")
    report["enemies"].append({"name": name, "mesh": read(mesh, "skeletal_mesh_asset"),
        "anim_class": read(mesh, "anim_class"), "mode": read(mesh, "animation_mode"),
        "properties": {p: read(obj, p) for p in ["m_pAttackMontage", "m_pDeathMontage", "m_pLightComboMontage",
        "m_pPowerSlamMontage", "m_pChargeRushMontage", "m_pBackStepMontage", "m_pPhaseTransitionMontage",
        "m_comboAttackMontages", "m_patrolWalkSpeed", "m_chaseRunSpeed", "ai_controller_class"]}})

paths = list(unreal.EditorAssetLibrary.list_assets("/Game/Assets/Enemy", recursive=True))
paths += list(unreal.EditorAssetLibrary.list_assets("/Game/Blueprints/Animations", recursive=True))
for path in paths:
    obj = unreal.load_asset(path)
    if isinstance(obj, unreal.AnimSequence):
        report["animations"].append({"path": path, "skeleton": read(obj, "skeleton"),
            "root_motion": read(obj, "enable_root_motion"), "root_lock": read(obj, "force_root_lock"),
            "length": read(obj, "sequence_length")})
    elif isinstance(obj, unreal.AnimMontage):
        report["animations"].append({"path": path, "skeleton": read(obj, "skeleton"),
            "slots": read(obj, "slot_anim_tracks"), "notifies": read(obj, "notifies")})
    elif isinstance(obj, unreal.BlendSpace):
        report["animations"].append({"path": path, "skeleton": read(obj, "skeleton"),
            "samples": [{"animation": read(s, "animation"), "value": read(s, "sample_value")} for s in obj.get_editor_property("sample_data")]})
    elif isinstance(obj, unreal.AnimBlueprint):
        report["blueprints"].append({"path": path, "skeleton": read(obj, "target_skeleton"),
            "dependencies": [str(p) for p in unreal.AssetRegistryHelpers.get_asset_registry().get_dependencies(path.split('.')[0], unreal.AssetRegistryDependencyOptions(True, True, False, False, False))]})

output = os.path.join(unreal.Paths.project_saved_dir(), "enemy_asset_audit.json")
with open(output, "w", encoding="utf-8") as stream:
    json.dump(report, stream, ensure_ascii=False, indent=2)
unreal.log("Enemy audit saved: " + output)
