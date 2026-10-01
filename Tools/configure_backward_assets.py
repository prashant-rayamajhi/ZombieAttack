import os
import shutil
import unreal

#各武器の前進側を残し、負の速度側へ後退クリップを追加する。
unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
folder = '/Game/Assets/Player/Animation/'
for weapon in ('Knife', 'Pistol', 'AR'):
    path = folder + 'BlendSpace/BS_' + weapon + '_Locomotion'
    blend = unreal.load_asset(path)
    filename = os.path.join(unreal.Paths.project_content_dir(), path.removeprefix('/Game/') + '.uasset')
    backup = os.path.join(unreal.Paths.project_saved_dir(), 'MovementBackup', os.path.basename(filename))
    os.makedirs(os.path.dirname(backup), exist_ok=True)
    if not os.path.exists(backup):
        shutil.copy2(filename, backup)
    params = list(blend.get_editor_property('blend_parameters'))
    params[0].set_editor_property('min', -600.0)
    blend.set_editor_property('blend_parameters', params)
    samples = [s for s in blend.get_editor_property('sample_data') if s.get_editor_property('sample_value').x >= 0.0]
    clip_name = 'Backwards_Rifle_Walk' if weapon == 'AR' else 'Pistol_Run_Backward'
    clip = unreal.load_asset(folder + 'AnimSequence/' + clip_name)
    if clip.get_editor_property('skeleton') != blend.get_editor_property('skeleton'):
        raise RuntimeError('Skeleton mismatch: ' + path)
    for speed in (60.0, 120.0, 180.0, 280.0, 600.0):
        sample = unreal.BlendSample()
        sample.set_editor_property('animation', clip)
        sample.set_editor_property('sample_value', unreal.Vector(-speed, 0.0, 0.0))
        #接地中の足の速度を測定した値に合わせ、歩幅に対して体だけが先へ滑るのを抑える。
        sample.set_editor_property('rate_scale', speed / (90.0 if weapon == 'AR' else 250.0))
        samples.append(sample)
    blend.set_editor_property('sample_data', samples)
    unreal.EditorAssetLibrary.save_loaded_asset(blend)
    unreal.log('BACKWARD CONFIGURED ' + path)
