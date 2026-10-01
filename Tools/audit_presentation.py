import json
import os
import unreal

#既存マップの露出と照明を記録し、手動設定を無条件に上書きしないための比較に使う。
report = {}
for level in ("GameLevel1", "GameStart", "GameClear", "GameOver"):
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/Map/" + level)
    rows = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        name = actor.get_class().get_name()
        if not any(word in name for word in ("Light", "Fog", "PostProcess", "DayNight", "Camera", "Sky")):
            continue
        row = {"actor": actor.get_actor_label(), "class": name, "location": str(actor.get_actor_location())}
        for field in ("Sun brightness", "Sun height", "Colors determined by sun position", "Zenith color", "Horizon color", "Cloud color", "Overall Color"):
            try:
                row[field] = str(actor.get_editor_property(field))
            except Exception:
                pass
        for field in ("m_dayLength", "m_daySunIntensity", "m_nightSunIntensity", "m_daySkyIntensity", "m_nightSkyIntensity", "m_gameplayExposureBias"):
            try:
                row[field] = actor.get_editor_property(field)
            except Exception:
                pass
        if isinstance(actor, unreal.PostProcessVolume):
            row["priority"] = actor.get_editor_property("priority")
            row["unbound"] = actor.get_editor_property("unbound")
            row["weight"] = actor.get_editor_property("blend_weight")
            settings = actor.get_editor_property("settings")
            row["settings"] = {}
            for field in ("auto_exposure_bias", "auto_exposure_min_brightness", "auto_exposure_max_brightness", "bloom_intensity", "vignette_intensity", "color_saturation"):
                row["settings"][field] = str(settings.get_editor_property(field))
        for component in actor.get_components_by_class(unreal.LightComponent):
            row["light"] = {"intensity": component.get_editor_property("intensity"), "color": str(component.get_editor_property("light_color"))}
        for component in actor.get_components_by_class(unreal.SkyLightComponent):
            row["sky"] = {"intensity": component.get_editor_property("intensity")}
        for component in actor.get_components_by_class(unreal.ExponentialHeightFogComponent):
            row["fog"] = {field: str(component.get_editor_property(field)) for field in ("fog_density", "fog_height_falloff")}
        rows.append(row)
    report[level] = rows
with open(os.path.join(unreal.Paths.project_saved_dir(), "Tests/presentation_audit.json"), "w", encoding="utf-8") as stream:
    json.dump(report, stream, ensure_ascii=False, indent=2)
