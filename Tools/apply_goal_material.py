import unreal

#配置済みの出口にも素材を保存し、ゲームを開始しなくてもエディタで質感を確認できるようにする。
unreal.EditorLoadingAndSavingUtils.load_map('/Game/Map/GameLevel1')
material = unreal.load_asset('/Game/Materials/Goal/M_EvacBeacon')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if not isinstance(actor, unreal.GoalActor):
        continue
    if actor.get_editor_property('m_beaconMaterial') != material:
        raise RuntimeError('Goal class has an unexpected material override')
    actor.set_editor_property('m_goalLightIntensity', 1200.0)
    for part in actor.get_components_by_class(unreal.StaticMeshComponent):
        if part.get_name().startswith('Beacon'):
            part.set_material(0, material)
    for light in actor.get_components_by_class(unreal.PointLightComponent):
        light.set_intensity(1200.0)
    unreal.log('GOAL MATERIAL APPLIED: ' + actor.get_actor_label())
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
