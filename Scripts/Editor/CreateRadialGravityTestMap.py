"""Genera TL_10_RadialGravity: el test level del spike de gravedad radial (ADR 0004, Fase 0).

Qué prueba este mapa, y sólo esto: que el personaje se sostiene de pie en cualquier orientación
sobre un cuerpo esférico. No es el terreno del juego, no tiene biomas y no debe recibir contenido.

Por qué el radio es 200 m y no un tier del plan. Los tiers de ingeniería (Lab 10 km, Target
500 km, Stress 2500 km) miden escala y precisión, y a 10 km de radio la media vuelta son 31 km:
imposible de recorrer a mano. Este mapa mide *orientación*, y para eso hace falta poder caminar
hasta el antípoda. A 200 m de radio la media vuelta son ~630 m. Es exactamente el uso de
laboratorio al que GUIA_ARTE_PLANETAS_BLENDER reclasificó sus radios de 150 m a 2 km.

Uso (NO usar -nullrhi, ver abajo):
    UnrealEditor-Cmd.exe Astraeon.uproject -RenderOffscreen -unattended -nosplash \
        -ExecutePythonScript=Scripts\\Editor\\CreateRadialGravityTestMap.py

Por qué -RenderOffscreen y no -nullrhi. Bajo -nullrhi, spawnear cualquier actor propio con
componente de malla mata el editor con EXCEPTION_INT_DIVIDE_BY_ZERO. Se comprobó que no es
específico de este mapa: `AAstraeonRegionMarker`, que lleva meses en el juego, crashea igual.
`CreateBootstrapMap.py` sobrevive con -nullrhi porque sólo spawnea clases del motor. Queda
anotado en KNOWN_ISSUES.
"""

import unreal

MAP_PACKAGE = "/Game/Maps/TL_10_RadialGravity"

# Radio del harness en centímetros. Ver la nota de arriba sobre por qué no es un tier del plan.
HARNESS_RADIUS_CM = 20000.0

# Altura a la que aparece el jugador sobre la superficie. Suficiente para no nacer incrustado en
# la esfera y poco para que la caída inicial no sea el primer efecto que se mide.
SPAWN_HEIGHT_CM = 120.0

CENTER = unreal.Vector(0.0, 0.0, 0.0)


def surface_point(direction, height_cm=0.0):
    """Punto sobre la superficie en una dirección dada. Misma fórmula que el harness en C++."""
    length = (direction.x ** 2 + direction.y ** 2 + direction.z ** 2) ** 0.5
    if length <= 0.0:
        return unreal.Vector(CENTER.x, CENTER.y, CENTER.z)

    scale = (HARNESS_RADIUS_CM + height_cm) / length
    return unreal.Vector(
        CENTER.x + direction.x * scale,
        CENTER.y + direction.y * scale,
        CENTER.z + direction.z * scale,
    )


def spawn_actor(actor_class, label, location, rotation=(0.0, 0.0, 0.0)):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class,
        location,
        unreal.Rotator(*rotation),
    )
    actor.set_actor_label(label)
    return actor


def spawn_marker(label, direction, color_scale):
    """Baliza sobre la superficie. Existe para tener a dónde caminar y contra qué comparar la
    orientación: si la cápsula se alinea bien, el jugador y la baliza comparten arriba local."""
    cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        return None

    location = surface_point(direction, 150.0)
    marker = unreal.EditorLevelLibrary.spawn_actor_from_object(cube, location)
    marker.set_actor_label(label)
    marker.set_actor_scale3d(unreal.Vector(*color_scale))

    # Orientada al arriba local, no al Z global: la baliza tiene que verse "de pie" desde el
    # jugador que llegue caminando, o no sirve como referencia de orientación.
    up = unreal.Vector(direction.x, direction.y, direction.z)
    marker.set_actor_rotation(unreal.MathLibrary.make_rot_from_z(up), False)
    return marker


def require_current_level(expected_package):
    """Aborta si el mundo del editor no es el que creamos.

    No es paranoia: `new_level` crea y guarda el nivel, pero el mundo activo del editor puede
    seguir siendo el mapa por defecto, y entonces los spawns caen en `L_AstraeonBootstrap` y el
    guardado posterior lo sobrescribe. Se detectó exactamente eso durante la Fase 0. Fallar aquí
    cuesta un mensaje; no fallar cuesta el mapa del juego.
    """
    world = unreal.EditorLevelLibrary.get_editor_world()
    actual = world.get_path_name() if world else "<sin mundo>"
    expected_name = expected_package.rsplit("/", 1)[-1]

    if expected_name not in actual:
        raise RuntimeError(
            "El mundo activo es '{}' y se esperaba '{}'. Se aborta antes de spawnear nada "
            "para no escribir en el mapa equivocado.".format(actual, expected_name)
        )
    unreal.log("Mundo activo verificado: {}".format(actual))


def main():
    unreal.EditorLevelLibrary.new_level(MAP_PACKAGE)
    require_current_level(MAP_PACKAGE)

    harness = spawn_actor(
        unreal.AstraeonPlanetGravityHarness,
        "PlanetGravityHarness",
        unreal.Vector(CENTER.x, CENTER.y, CENTER.z),
    )
    harness.set_editor_property("planet_radius_cm", HARNESS_RADIUS_CM)
    harness.set_editor_property("surface_gravity_ms2", 9.81)

    # El jugador nace en el polo norte, donde el arriba local coincide con el Z global. Es el
    # único punto donde un fallo de alineación NO se nota, y por eso es el punto de partida
    # correcto: todo lo que se camine desde aquí revela el defecto.
    spawn_actor(
        unreal.PlayerStart,
        "PlayerStart_NorthPole",
        surface_point(unreal.Vector(0.0, 0.0, 1.0), SPAWN_HEIGHT_CM),
    )

    # Las cuatro balizas del criterio de salida: ecuador en dos meridianos, y el antípoda, que es
    # el caso que la puerta de la Fase 0 exige comprobar a mano.
    spawn_marker("Marker_EquatorX", unreal.Vector(1.0, 0.0, 0.0), (1.0, 1.0, 4.0))
    spawn_marker("Marker_EquatorY", unreal.Vector(0.0, 1.0, 0.0), (1.0, 1.0, 4.0))
    spawn_marker("Marker_Antipode", unreal.Vector(0.0, 0.0, -1.0), (2.0, 2.0, 6.0))
    spawn_marker("Marker_Diagonal", unreal.Vector(1.0, 1.0, 1.0), (1.0, 1.0, 4.0))

    # Luz direccional lejana y skylight: sin ambiente, la mitad no iluminada de la esfera queda
    # negra y no se puede juzgar si el personaje está de pie.
    spawn_actor(unreal.DirectionalLight, "Sun_TestKey",
                unreal.Vector(0.0, 0.0, HARNESS_RADIUS_CM * 3.0), (-45.0, 0.0, 0.0))
    spawn_actor(unreal.SkyLight, "Sky_TestAmbient",
                unreal.Vector(0.0, 0.0, HARNESS_RADIUS_CM * 2.0))

    unreal.EditorLoadingAndSavingUtils.save_current_level()
    unreal.EditorAssetLibrary.save_asset(MAP_PACKAGE, only_if_is_dirty=False)
    unreal.log("TL_10_RadialGravity generado en {} (radio {:.0f} cm)".format(MAP_PACKAGE, HARNESS_RADIUS_CM))


if __name__ == "__main__":
    main()
