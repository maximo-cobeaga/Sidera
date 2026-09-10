"""Phase 2 TL_12: 50 km planet, patches only, no near collision. Run with -RenderOffscreen.
Reuses TL_11's generated checker; only writes its own map.
"""
import unreal as u

MAP = '/Game/Maps/TL_12_PatchLOD'
MATERIAL = '/Game/Astraeon/Tests/Planet/M_Planet_Checker'
RADIUS_CM = 5000000.0


def require_world():
    world = u.EditorLevelLibrary.get_editor_world()
    if world.get_outermost().get_name() != MAP:
        raise RuntimeError('Wrong active map: ' + world.get_path_name())


def configure(planet, material):
    planet.set_editor_property('radius_cm', RADIUS_CM)
    planet.set_editor_property('near_collision', False)
    planet.set_editor_property('surface_material', material)
    if not planet.rebuild():
        raise RuntimeError('Planet rebuild failed')


def main():
    material = u.EditorAssetLibrary.load_asset(MATERIAL)
    if not material or u.EditorAssetLibrary.get_metadata_tag(material, 'AstraeonGenerator') != 'CubeSphereLabV1':
        raise RuntimeError('Run CreateCubeSphereTestMap.py first: missing generated checker ' + MATERIAL)
    if u.EditorAssetLibrary.does_asset_exist(MAP):
        u.EditorLevelLibrary.load_level(MAP)
        require_world()
        actors = [a for a in u.EditorLevelLibrary.get_all_level_actors() if isinstance(a, u.AstraeonPlanetRuntime)]
        if len(actors) != 1:
            raise RuntimeError('Existing map has unexpected runtime count')
        planet = actors[0]
        configure(planet, material)
    else:
        u.EditorLevelLibrary.new_level(MAP)
        require_world()
        planet = u.EditorLevelLibrary.spawn_actor_from_class(u.AstraeonPlanetRuntime, u.Vector(0, 0, 0))
        planet.set_actor_label('Planet_PatchLOD_50km')
        configure(planet, material)
        start = u.EditorLevelLibrary.spawn_actor_from_class(u.PlayerStart, planet.get_surface_point_cm(u.Vector(0, 0, 1), 150.0))
        start.set_actor_label('Start_PatchLOD')
        sun = u.EditorLevelLibrary.spawn_actor_from_class(u.DirectionalLight, u.Vector(0, 0, 0), u.Rotator(-45, 0, 0))
        sun.light_component.set_mobility(u.ComponentMobility.MOVABLE)
    require_world()
    if not u.EditorLoadingAndSavingUtils.save_current_level():
        raise RuntimeError('Map save failed')
    u.EditorLevelLibrary.load_level(MAP)
    require_world()
    actors = [a for a in u.EditorLevelLibrary.get_all_level_actors() if isinstance(a, u.AstraeonPlanetRuntime)]
    if len(actors) != 1 or actors[0].get_editor_property('near_collision') or actors[0].get_editor_property('radius_cm') != RADIUS_CM:
        raise RuntimeError('Reload validation failed')
    u.log('ASTRAEON_PATCH_LOD_MAP: PASS ' + MAP)


main()
