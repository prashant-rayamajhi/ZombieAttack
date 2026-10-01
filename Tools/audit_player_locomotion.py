import unreal

#後退アニメーションと移動用BlendSpaceの接続を、変更前に記録する。
unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
for path in unreal.EditorAssetLibrary.list_assets('/Game/Blueprints/Animations', recursive=True):
    obj = unreal.load_asset(path)
    if isinstance(obj, unreal.BlendSpace):
        unreal.log('LOCOMOTION ' + path)
        for field in ('blend_parameters', 'sample_data'):
            try:
                for item in obj.get_editor_property(field):
                    unreal.log(field + ': ' + item.export_text())
            except Exception as error:
                unreal.log(str(error))
for path in ['/Game/Blueprints/Animations/BPA_Player']:
    obj = unreal.load_asset(path)
    task = unreal.AssetExportTask()
    task.object = obj
    task.filename = unreal.Paths.project_saved_dir() + 'Tests/PlayerAnim.copy'
    task.automated = True
    task.prompt = False
    unreal.Exporter.run_asset_export_task(task)
