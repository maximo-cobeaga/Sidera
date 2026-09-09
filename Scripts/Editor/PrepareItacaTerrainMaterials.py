"""Import generated albedos and build reproducible, physically scaled materials."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.project_dir()).resolve()
DEST = '/Game/Astraeon/Art/ItacaTerrain'
LIB = u.MaterialEditingLibrary
ASSETS = u.EditorAssetLibrary


def node(mat, cls, **props):
    result = LIB.create_material_expression(mat, cls)
    for key, value in props.items():
        result.set_editor_property(key, value)
    return result


def scalar(mat, value, prop):
    LIB.connect_material_property(node(mat, u.MaterialExpressionConstant, r=value), '', prop)


def material(name):
    path = DEST + '/' + name
    mat = ASSETS.load_asset(path) if ASSETS.does_asset_exist(path) else None
    if mat:
        if ASSETS.get_metadata_tag(mat, 'AstraeonGenerator') != 'ItacaTerrainV1':
            raise RuntimeError('Refusing unrelated material: ' + path)
        LIB.delete_all_material_expressions(mat)
    else:
        mat = u.AssetToolsHelpers.get_asset_tools().create_asset(name, DEST, u.Material, u.MaterialFactoryNew())
    ASSETS.set_metadata_tag(mat, 'AstraeonGenerator', 'ItacaTerrainV1')
    return mat


def save(mat):
    LIB.recompile_material(mat)
    if not ASSETS.save_loaded_asset(mat):
        raise RuntimeError('Save failed: ' + mat.get_name())


def main():
    u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.PNG 0')
    textures = {}
    report = {'passed': False, 'textures': [], 'materials': []}
    for name in ('T_Regolith_BC', 'T_Strata_BC', 'T_ItacaPanel_BC'):
        source = ROOT / 'ContentPipeline/Textures/ItacaTerrain' / (name + '.png')
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        path = DEST + '/' + name
        if ASSETS.does_asset_exist(path):
            tex = ASSETS.load_asset(path)
            if ASSETS.get_metadata_tag(tex, 'AstraeonSourceHash') != digest:
                raise RuntimeError('Source changed: explicit review required for ' + path)
        else:
            task = u.AssetImportTask()
            task.set_editor_property('factory', u.TextureFactory())
            for key, value in dict(filename=str(source), destination_path=DEST, destination_name=name,
                                   automated=True, save=False, replace_existing=False).items():
                task.set_editor_property(key, value)
            u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            tex = ASSETS.load_asset(path)
        if not isinstance(tex, u.Texture2D):
            raise RuntimeError('Missing texture: ' + path)
        tex.set_editor_property('srgb', True)
        # Generated source is NPOT. Stretch at build time, never pad a tile with a border.
        tex.set_editor_property('power_of_two_mode', u.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
        tex.set_editor_property('max_texture_size', 2048)
        tex.set_editor_property('address_x', u.TextureAddress.TA_WRAP)
        tex.set_editor_property('address_y', u.TextureAddress.TA_WRAP)
        ASSETS.set_metadata_tag(tex, 'AstraeonSourceHash', digest)
        ASSETS.save_loaded_asset(tex)
        textures[name] = tex
        report['textures'].append({'path': path, 'sha256': digest})

    expected = ['M_Terrain_Geology', 'M_Itaca_Panel', 'M_Itaca_Deck', 'M_Itaca_Guide', 'M_Itaca_Trim']
    existing = [ASSETS.does_asset_exist(DEST + '/' + name) for name in expected]
    if any(existing) and not all(existing):
        raise RuntimeError('Partial material set; inspect before rebuilding')
    if not all(existing):
        terrain = material('M_Terrain_Geology')
        terrain.set_editor_property('used_with_instanced_static_meshes', True)
        custom = node(terrain, u.MaterialExpressionCustom, output_type=u.CustomMaterialOutputType.CMOT_FLOAT3)
        inputs = []
        for name in ('Soil', 'Rock', 'P', 'N'):
            item = u.CustomInput()
            item.set_editor_property('input_name', name)
            inputs.append(item)
        custom.set_editor_property('inputs', inputs)
        custom.set_editor_property('code', '''
    float3 w = pow(abs(N), 4.0); w /= max(w.x+w.y+w.z, 0.001);
    float3 p = P / 200.0; // centimetres: one tile spans two metres
    float3 rock = Texture2DSample(Rock, RockSampler, p.yz).rgb*w.x
                + Texture2DSample(Rock, RockSampler, p.xz).rgb*w.y
                + Texture2DSample(Rock, RockSampler, p.xy).rgb*w.z;
    float3 soil = Texture2DSample(Soil, SoilSampler, p.xy).rgb;
    float macro = Texture2DSample(Soil, SoilSampler, P.xy/3700.0).r;
    float exposure = saturate((P.z-180.0)/2200.0 + (1.0-abs(N.z)));
    return lerp(soil, rock, exposure) * lerp(0.82, 1.08, macro);
    ''')
        for name, tex in (('Soil', textures['T_Regolith_BC']), ('Rock', textures['T_Strata_BC'])):
            LIB.connect_material_expressions(node(terrain, u.MaterialExpressionTextureObject, texture=tex), '', custom, name)
        LIB.connect_material_expressions(node(terrain, u.MaterialExpressionWorldPosition), '', custom, 'P')
        LIB.connect_material_expressions(node(terrain, u.MaterialExpressionVertexNormalWS), '', custom, 'N')
        LIB.connect_material_property(custom, '', u.MaterialProperty.MP_BASE_COLOR)
        scalar(terrain, 0.92, u.MaterialProperty.MP_ROUGHNESS)
        save(terrain)

        for name, tint, metallic in (('M_Itaca_Panel', (1., 1., 1.), 0.25),
                                      ('M_Itaca_Deck', (0.32, 0.39, 0.43), 0.45)):
            mat = material(name)
            tex = node(mat, u.MaterialExpressionTextureSample, texture=textures['T_ItacaPanel_BC'])
            multiply = node(mat, u.MaterialExpressionMultiply)
            LIB.connect_material_expressions(tex, 'RGB', multiply, 'A')
            color = node(mat, u.MaterialExpressionConstant3Vector, constant=u.LinearColor(*tint, 1.))
            LIB.connect_material_expressions(color, '', multiply, 'B')
            LIB.connect_material_property(multiply, '', u.MaterialProperty.MP_BASE_COLOR)
            scalar(mat, metallic, u.MaterialProperty.MP_METALLIC)
            scalar(mat, 0.72, u.MaterialProperty.MP_ROUGHNESS)
            save(mat)

        for name, color, emission in (('M_Itaca_Guide', (0.08, 0.65, 0.8), 2.),
                                        ('M_Itaca_Trim', (0.04, 0.065, 0.08), 0.)):
            mat = material(name)
            c = node(mat, u.MaterialExpressionConstant3Vector, constant=u.LinearColor(*color, 1.))
            LIB.connect_material_property(c, '', u.MaterialProperty.MP_BASE_COLOR)
            if emission:
                e = node(mat, u.MaterialExpressionConstant3Vector, constant=u.LinearColor(*(v*emission for v in color), 1.))
                LIB.connect_material_property(e, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
            scalar(mat, 0.65, u.MaterialProperty.MP_ROUGHNESS)
            save(mat)
    for mesh_name, mat_name in (('Wall_200', 'Panel'), ('Floor_200', 'Deck'), ('ARGOSConsole', 'Panel')):
        mesh = ASSETS.load_asset('/Game/Astraeon/Art/Blockouts/Itaca/SM_Itaca_' + mesh_name + '_Blockout')
        if not mesh:
            raise RuntimeError('Missing Itaca mesh: ' + mesh_name)
        mesh.set_material(0, ASSETS.load_asset(DEST + '/M_Itaca_' + mat_name))
        ASSETS.save_loaded_asset(mesh)
    report['materials'] = list(ASSETS.list_assets(DEST))
    report['passed'] = True
    (ROOT / 'ContentPipeline/reports/itaca_terrain_materials.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    u.log('ASTRAEON_ITACA_TERRAIN_MATERIALS: PASS')


if __name__ == '__main__':
    main()
