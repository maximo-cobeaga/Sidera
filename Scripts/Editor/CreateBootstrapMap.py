import unreal

MAP_PACKAGE = "/Game/Maps/L_AstraeonBootstrap"


def spawn_actor(actor_class, label, location, rotation=(0.0, 0.0, 0.0)):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class,
        unreal.Vector(*location),
        unreal.Rotator(*rotation),
    )
    actor.set_actor_label(label)
    return actor


def main():
    unreal.EditorLevelLibrary.new_level(MAP_PACKAGE)

    spawn_actor(unreal.PlayerStart, "PlayerStart_ItacaBootstrap", (0.0, 0.0, 120.0), (0.0, 0.0, 0.0))
    spawn_actor(unreal.DirectionalLight, "Sun_TestKey", (0.0, 0.0, 500.0), (-45.0, 0.0, 0.0))
    spawn_actor(unreal.SkyLight, "Sky_TestAmbient", (0.0, 0.0, 300.0))

    cube_asset = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    if cube_asset:
        floor = unreal.EditorLevelLibrary.spawn_actor_from_object(cube_asset, unreal.Vector(0.0, 0.0, -50.0))
        floor.set_actor_label("Floor_DeterministicRegionStub")
        floor.set_actor_scale3d(unreal.Vector(20.0, 20.0, 0.5))

        wall_specs = [
            ("Wall_North", (0.0, 1000.0, 100.0), (20.0, 0.5, 3.0)),
            ("Wall_South", (0.0, -1000.0, 100.0), (20.0, 0.5, 3.0)),
            ("Wall_East", (1000.0, 0.0, 100.0), (0.5, 20.0, 3.0)),
            ("Wall_West", (-1000.0, 0.0, 100.0), (0.5, 20.0, 3.0)),
        ]
        for label, location, scale in wall_specs:
            wall = unreal.EditorLevelLibrary.spawn_actor_from_object(cube_asset, unreal.Vector(*location))
            wall.set_actor_label(label)
            wall.set_actor_scale3d(unreal.Vector(*scale))

    unreal.EditorLoadingAndSavingUtils.save_current_level()
    unreal.EditorAssetLibrary.save_asset(MAP_PACKAGE, only_if_is_dirty=False)
    unreal.log("Astraeon bootstrap map generated at {}".format(MAP_PACKAGE))


if __name__ == "__main__":
    main()
