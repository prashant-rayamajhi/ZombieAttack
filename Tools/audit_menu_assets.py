import unreal

unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
cls = unreal.load_class(None, '/Game/Blueprints/Player/Actor/BP_PlayerChara.BP_PlayerChara_C')
player = unreal.get_default_object(cls)
unreal.log('MENU_PLAYER_MESH=' + player.mesh.skeletal_mesh_asset.get_path_name())
lamp = unreal.load_asset('/Game/Fab/Street_Lamp/street_lamp')
unreal.log('MENU_LAMP_BOUNDS=' + str(lamp.get_bounds()))
