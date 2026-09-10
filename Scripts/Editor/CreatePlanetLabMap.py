"""Phase 2 planet labs beyond TL_11/TL_12, one table entry per map. Run with -RenderOffscreen:
  -ExecutePythonScript="Scripts/Editor/CreatePlanetLabMap.py TL_13_CollisionRing"
Reuses TL_11's generated checker; only writes the map it is asked for.
"""
import sys
import unreal as u

MATERIAL = '/Game/Astraeon/Tests/Planet/M_Planet_Checker'
LABS = {
    # Walkable 50 km planet: collision can only come from the ring. Spawn on a cube corner so
    # the first steps already cross face seams.
    'TL_13_CollisionRing': dict(radius_cm=5000000.0, near_collision=True, spawn=(1.0, 1.0, 1.0)),
    # Target tier, 500 km, walkable: local frames keep physics and rendering near the origin.
    # The Stress tier (2500 km) is the same map with -AstraeonPlanetRadiusCm=250000000.
    'TL_14_FrameTransition': dict(radius_cm=50000000.0, near_collision=True, spawn=(1.0, 0.0, 1.0)),
}


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else ''
    if name not in LABS:
        raise RuntimeError('Unknown planet lab: %r; expected one of %s' % (name, sorted(LABS)))
    lab = LABS[name]
    path = '/Game/Maps/' + name
    material = u.EditorAssetLibrary.load_asset(MATERIAL)
    if not material or u.EditorAssetLibrary.get_metadata_tag(material, 'AstraeonGenerator') != 'CubeSphereLabV1':
        raise RuntimeError('Run CreateCubeSphereTestMap.py first: missing generated checker ' + MATERIAL)

    def require_world():
        if u.EditorLevelLibrary.get_editor_world().get_outermost().get_name() != path:
            raise RuntimeError('Wrong active map for ' + path)

    def runtimes():
        return [a for a in u.EditorLevelLibrary.get_all_level_actors() if isinstance(a, u.AstraeonPlanetRuntime)]

    spawn = u.Vector(*lab['spawn'])
    if u.EditorAssetLibrary.does_asset_exist(path):
        u.EditorLevelLibrary.load_level(path)
        require_world()
        found = runtimes()
        if len(found) != 1:
            raise RuntimeError('Existing map has unexpected runtime count')
        planet = found[0]
    else:
        u.EditorLevelLibrary.new_level(path)
        require_world()
        planet = u.EditorLevelLibrary.spawn_actor_from_class(u.AstraeonPlanetRuntime, u.Vector(0, 0, 0))
        planet.set_actor_label('Planet_' + name)
        sun = u.EditorLevelLibrary.spawn_actor_from_class(u.DirectionalLight, u.Vector(0, 0, 0), u.Rotator(-45, 0, 0))
        sun.light_component.set_mobility(u.ComponentMobility.MOVABLE)
    planet.set_editor_property('radius_cm', lab['radius_cm'])
    planet.set_editor_property('near_collision', lab['near_collision'])
    planet.set_editor_property('spawn_direction', spawn)
    planet.set_editor_property('surface_material', material)
    if not planet.rebuild():
        raise RuntimeError('Planet rebuild failed')
    starts = [a for a in u.EditorLevelLibrary.get_all_level_actors() if isinstance(a, u.PlayerStart)]
    start = starts[0] if starts else u.EditorLevelLibrary.spawn_actor_from_class(u.PlayerStart, u.Vector(0, 0, 0))
    start.set_actor_label('Start_' + name)
    start.set_actor_location(planet.get_surface_point_cm(spawn, 150.0), False, False)
    require_world()
    if not u.EditorLoadingAndSavingUtils.save_current_level():
        raise RuntimeError('Map save failed')
    u.EditorLevelLibrary.load_level(path)
    require_world()
    found = runtimes()
    if len(found) != 1 or found[0].get_editor_property('radius_cm') != lab['radius_cm'] \
            or found[0].get_editor_property('near_collision') != lab['near_collision']:
        raise RuntimeError('Reload validation failed')
    u.log('ASTRAEON_PLANET_LAB_MAP: PASS ' + path)


main()
