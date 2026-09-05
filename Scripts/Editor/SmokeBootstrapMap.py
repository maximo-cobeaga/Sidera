import unreal

MAP_PACKAGE = "/Game/Maps/L_AstraeonBootstrap"


def main():
    loaded = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PACKAGE)
    if not loaded:
        raise RuntimeError("Unable to load {}".format(MAP_PACKAGE))

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_subsystem.get_all_level_actors()
    labels = [actor.get_actor_label() for actor in actors]
    required = {"PlayerStart_ItacaBootstrap", "Floor_DeterministicRegionStub"}
    missing = sorted(required.difference(labels))
    if missing:
        raise RuntimeError("Bootstrap map missing required actors: {}".format(", ".join(missing)))

    unreal.log("Astraeon bootstrap map smoke passed: {} actors loaded from {}".format(len(actors), MAP_PACKAGE))


if __name__ == "__main__":
    main()
