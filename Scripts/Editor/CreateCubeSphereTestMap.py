"""Phase 1 TL_11 + triplanar checker. Run with -RenderOffscreen, not -nullrhi.
Only writes its own map and generated material; never the current user map.
"""
import unreal as u

MAP = '/Game/Maps/TL_11_CubeSphereClosed'
DEST = '/Game/Astraeon/Tests/Planet'
TAG = 'CubeSphereLabV1'


def make_material():
    path = DEST + '/M_Planet_Checker'
    mat = u.EditorAssetLibrary.load_asset(path)
    if mat:
        if u.EditorAssetLibrary.get_metadata_tag(mat, 'AstraeonGenerator') != TAG:
            raise RuntimeError('Unowned material: ' + path)
        return mat
    mat = u.AssetToolsHelpers.get_asset_tools().create_asset('M_Planet_Checker', DEST, u.Material, u.MaterialFactoryNew())
    lib = u.MaterialEditingLibrary
    custom = lib.create_material_expression(mat, u.MaterialExpressionCustom)
    custom.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs = []
    for name in ('P', 'N'):
        entry = u.CustomInput()
        entry.set_editor_property('input_name', name)
        inputs.append(entry)
    custom.set_editor_property('inputs', inputs)
    custom.set_editor_property('code', '''
float3 p=P/500.0;
float3 w=pow(abs(N),4.0); w/=max(w.x+w.y+w.z,0.001);
float a=fmod(abs(floor(p.y)+floor(p.z)),2.0);
float b=fmod(abs(floor(p.x)+floor(p.z)),2.0);
float c=fmod(abs(floor(p.x)+floor(p.y)),2.0);
return lerp(float3(0.06,0.12,0.17),float3(0.32,0.47,0.50),a*w.x+b*w.y+c*w.z);
''')
    for cls, name in ((u.MaterialExpressionWorldPosition,'P'), (u.MaterialExpressionVertexNormalWS,'N')):
        lib.connect_material_expressions(lib.create_material_expression(mat,cls),'',custom,name)
    lib.connect_material_property(custom,'',u.MaterialProperty.MP_BASE_COLOR)
    # Low emission makes the unlit hemisphere inspectable in this geometry laboratory.
    lib.connect_material_property(custom,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    roughness = lib.create_material_expression(mat,u.MaterialExpressionConstant)
    roughness.set_editor_property('r',0.9)
    lib.connect_material_property(roughness,'',u.MaterialProperty.MP_ROUGHNESS)
    u.EditorAssetLibrary.set_metadata_tag(mat,'AstraeonGenerator',TAG)
    lib.recompile_material(mat)
    if not u.EditorAssetLibrary.save_loaded_asset(mat):
        raise RuntimeError('Material save failed')
    return mat


def require_world():
    world = u.EditorLevelLibrary.get_editor_world()
    if world.get_outermost().get_name() != MAP:
        raise RuntimeError('Wrong active map: '+world.get_path_name())


def main():
    material = make_material()
    if u.EditorAssetLibrary.does_asset_exist(MAP):
        u.EditorLevelLibrary.load_level(MAP)
        require_world()
        actors = [a for a in u.EditorLevelLibrary.get_all_level_actors() if isinstance(a,u.AstraeonPlanetRuntime)]
        if len(actors)!=1:
            raise RuntimeError('Existing map has unexpected runtime count')
        planet = actors[0]
    else:
        u.EditorLevelLibrary.new_level(MAP)
        require_world()
        planet = u.EditorLevelLibrary.spawn_actor_from_class(u.AstraeonPlanetRuntime,u.Vector(0,0,0))
        planet.set_actor_label('Planet_CubeSphere_Lab')
        planet.set_editor_property('surface_material',material)
        if not planet.rebuild():
            raise RuntimeError('Planet rebuild failed')
        start = u.EditorLevelLibrary.spawn_actor_from_class(u.PlayerStart,planet.get_surface_point_cm(u.Vector(0,0,1),150.0))
        start.set_actor_label('Start_CubeSphere')
        sun = u.EditorLevelLibrary.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,60000),u.Rotator(-45,0,0))
        sun.light_component.set_mobility(u.ComponentMobility.MOVABLE)
    planet.set_editor_property('surface_material',material)
    if not planet.rebuild():
        raise RuntimeError('Planet rebuild failed')
    require_world()
    if not u.EditorLoadingAndSavingUtils.save_current_level():
        raise RuntimeError('Map save failed')
    u.EditorLevelLibrary.load_level(MAP)
    require_world()
    count = len([a for a in u.EditorLevelLibrary.get_all_level_actors() if isinstance(a,u.AstraeonPlanetRuntime)])
    if count!=1:
        raise RuntimeError('Reload validation failed')
    u.log('ASTRAEON_CUBE_SPHERE_MAP: PASS '+MAP)


main()
