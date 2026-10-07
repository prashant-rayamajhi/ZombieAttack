import os
import shutil
import unreal

#元のマップを退避し、照明の調整前へ戻せるようにする。
source = os.path.join(unreal.Paths.project_content_dir(), "Map/GameLevel1.umap")
backup = os.path.join(unreal.Paths.project_saved_dir(), "PresentationBackup/BeforeDawn20261004/GameLevel1.umap")
os.makedirs(os.path.dirname(backup), exist_ok=True)
if not os.path.exists(backup):
    shutil.copy2(source, backup)
unreal.EditorLoadingAndSavingUtils.load_map("/Game/Map/GameLevel1")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
for actor in actors:
    if isinstance(actor, unreal.PostProcessVolume):
        if actor.get_actor_label() == "PostProcessVolume2":
            #重複していた全域露出は削除せず無効にし、設定の戻し先として残す。
            actor.set_editor_property("blend_weight", 0.0)
        elif actor.get_actor_label() == "PostProcessVolume":
            settings = actor.get_editor_property("settings")
            for field, value in (("auto_exposure_bias", -0.2), ("vignette_intensity", 0.22), ("bloom_intensity", 0.15)):
                settings.set_editor_property("override_" + field, True)
                settings.set_editor_property(field, value)
            settings.set_editor_property("override_color_saturation", True)
            settings.set_editor_property("color_saturation", unreal.Vector4(0.85, 0.85, 0.85, 1.0))
            #空の明るさを押し上げず、木陰にいる敵の服と顔の階調を持ち上げる。
            settings.set_editor_property("override_color_gamma_shadows", True)
            settings.set_editor_property("color_gamma_shadows", unreal.Vector4(1.08, 1.08, 1.08, 1.0))
            actor.set_editor_property("settings", settings)
            actor.set_editor_property("priority", 10.0)
    for fog in actor.get_components_by_class(unreal.ExponentialHeightFogComponent):
        #遠景の霞を残しながら、近くの敵の輪郭が白い霧へ埋もれるのを抑える。
        fog.set_editor_property("fog_density", 0.12)
        fog.set_editor_property("fog_height_falloff", 0.08)
        #高い地形でも霧が地面の下へ沈まず、木の間に層が残る高さへ合わせる。
        actor.set_actor_location(unreal.Vector(0.0, 0.0, 400.0), False, False)
        #近くの敵は霧で覆わず、奥の木々を薄く霞ませて前後の距離を読みやすくする。
        fog.set_editor_property("start_distance", 900.0)
        fog.set_editor_property("fog_max_opacity", 0.82)
        #空まで白く光っていた霧の発光色を落とし、照明自体を暗くせず遠景を締める。
        fog.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.035, 0.045, 0.055, 1.0))
        fog.set_editor_property("directional_inscattering_luminance", unreal.LinearColor(0.07, 0.055, 0.04, 1.0))
    for light in actor.get_components_by_class(unreal.PointLightComponent):
        #朝方にも街灯の目印は残し、足元だけ白飛びして敵が暗く見える照度差を抑える。
        intensity = light.get_editor_property("intensity")
        if intensity >= 100.0:
            light.set_editor_property("intensity", min(intensity, 180.0))
    for sky in actor.get_components_by_class(unreal.SkyLightComponent):
        #街灯の届かない場所にも弱い環境光を残し、敵の輪郭が黒く潰れるのを抑える。
        sky.set_mobility(unreal.ComponentMobility.MOVABLE)
        sky.set_editor_property('real_time_capture', True)
        sky.set_editor_property("intensity", 1.8)
#月光の逆側から弱い反射光を足し、敵がこちらを向いたときの黒潰れを防ぐ。
fill = next((actor for actor in actors if actor.get_actor_label() == 'ForestSoftFill'), None)
key = next((actor for actor in actors if isinstance(actor, unreal.DirectionalLight) and actor != fill), None)
if key:
    #低い暖色の朝日と寒色の環境光を分け、夜明けの空気と敵の輪郭を両立する。
    key.set_actor_rotation(unreal.Rotator(-12.0, 110.0, 0.0), False)
    sunlight = key.get_component_by_class(unreal.DirectionalLightComponent)
    sunlight.set_editor_property('intensity', 1.2)
    sunlight.set_editor_property('forward_shading_priority', 1)
    sunlight.set_light_color(unreal.LinearColor(1.0, 0.78, 0.57, 1.0))
if not fill:
    fill = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 1000.0))
    fill.set_actor_label('ForestSoftFill')
rotation = key.get_actor_rotation() if key else unreal.Rotator(-35.0, 135.0, 0.0)
fill.set_actor_rotation(unreal.Rotator(-25.0, rotation.yaw + 180.0, 0.0), False)
light = fill.get_component_by_class(unreal.DirectionalLightComponent)
light.set_editor_property('intensity', 0.5)
light.set_editor_property('forward_shading_priority', 0)
light.set_editor_property('cast_shadows', False)
light.set_editor_property('indirect_lighting_intensity', 0.0)
light.set_editor_property('volumetric_scattering_intensity', 0.0)
light.set_light_color(unreal.LinearColor(0.65, 0.73, 0.85, 1.0))
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
