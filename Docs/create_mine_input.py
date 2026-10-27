import unreal

path = '/Game/VehicleTemplate/Input/Actions/IA_PlaceMine'
action = unreal.load_asset(path)
if action is None:
    action = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'IA_PlaceMine', '/Game/VehicleTemplate/Input/Actions',
        unreal.InputAction, unreal.InputAction_Factory())
    action.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
    unreal.EditorAssetLibrary.save_loaded_asset(action)
if action is None:
    raise RuntimeError('IA_PlaceMine creation failed')
unreal.log('CX: IA_PlaceMine asset ready')
