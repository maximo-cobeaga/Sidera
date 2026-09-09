"""Persist the already validated original kit. Existing assets require matching provenance."""
import hashlib
import json
from pathlib import Path
import sys
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0, str(ROOT / 'Scripts/Editor'))
import ValidateArtBlockoutImport as validation

DEST = '/Game/Astraeon/Art/Blockouts/Itaca'
config = json.loads((ROOT / 'Tools/Blender/configs/itaca_blockout.json').read_text(encoding='utf-8'))
paths = [DEST + '/' + spec['id'] for spec in config['assets']]
existing = [unreal.EditorAssetLibrary.does_asset_exist(path) for path in paths]
if any(existing) and not all(existing):
    raise RuntimeError('Partial destination: inspect assets before attempting replacement')
if not any(existing):
    validation.DESTINATION = DEST
    validation.main()

results = []
for spec, path in zip(config['assets'], paths):
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    source = ROOT / 'ContentPipeline/Generated/ItacaBlockout' / (spec['id'] + '.fbx')
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    if all(existing) and unreal.EditorAssetLibrary.get_metadata_tag(mesh, 'AstraeonSourceHash') != digest:
        raise RuntimeError('Source changed; explicit reimport required: ' + path)
    mat_path = DEST + '/' + spec['material']
    mat = unreal.EditorAssetLibrary.load_asset(mat_path) if unreal.EditorAssetLibrary.does_asset_exist(mat_path) else None
    if mat is None:
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(spec['material'], DEST, unreal.Material, unreal.MaterialFactoryNew())
        color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
        color.set_editor_property('constant', unreal.LinearColor(*spec['color'], 1.0))
        unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
        roughness = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant)
        roughness.set_editor_property('r', 0.65)
        unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mat)
    mesh.set_material(0, mat)
    # Gameplay collision is explicitly assembled in the C++ interior, never a hull across the door.
    unreal.EditorAssetLibrary.set_metadata_tag(mesh, 'AstraeonSourceHash', digest)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    results.append({'id':spec['id'], 'asset':path, 'source_sha256':digest, 'saved':True})
(ROOT / 'ContentPipeline/reports/itaca_persistent_assets.json').write_text(json.dumps({'passed':True,'assets':results}, indent=2) + '\n', encoding='utf-8')
unreal.log('ITACA_PERSISTENT_ASSETS: PASS')
