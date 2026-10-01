import unreal

#同梱済みの金属テクスチャを使い、出口の目印へ塗装された金属の質感を付ける。
path = '/Game/Materials/Goal/M_EvacBeacon'
material = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if not material:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_EvacBeacon', '/Game/Materials/Goal', unreal.Material, unreal.MaterialFactoryNew())
    edit = unreal.MaterialEditingLibrary
    texture = edit.create_material_expression(material, unreal.MaterialExpressionTextureSample, -600, 0)
    texture.set_editor_property('texture', unreal.load_asset('/Game/ModularBuildingSet/Textures/beam_window_supports_metal_c'))
    tint = edit.create_material_expression(material, unreal.MaterialExpressionMultiply, -300, 0)
    tint.set_editor_property('const_b', 0.45)
    edit.connect_material_expressions(texture, 'RGB', tint, 'A')
    edit.connect_material_property(tint, '', unreal.MaterialProperty.MP_BASE_COLOR)
    for prop, value, y in [(unreal.MaterialProperty.MP_METALLIC, 0.65, 160), (unreal.MaterialProperty.MP_ROUGHNESS, 0.55, 240)]:
        scalar = edit.create_material_expression(material, unreal.MaterialExpressionConstant, -300, y)
        scalar.set_editor_property('r', value)
        edit.connect_material_property(scalar, '', prop)
    glow = edit.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, 400)
    glow.set_editor_property('parameter_name', 'BeaconColor')
    glow.set_editor_property('default_value', unreal.LinearColor(0.08, 0.006, 0.003, 1.0))
    edit.connect_material_property(glow, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    edit.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
