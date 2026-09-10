# Traspaso — Fase 2 cerrada, Fase 3 abierta, 2026-09-10

## Qué leer al reanudar

1. `AGENTS.md`.
2. `Docs/PHASE_STATUS.md`: Fase 2 PASADA, Fase 3 activa, lista de cierre.
3. `Docs/evidencia/PUERTA_FASE_2.md`: los ocho criterios con su evidencia.
4. `Docs/DECISIONS.md`, entradas P2.3 a P2.7, y `Docs/ADR/0006-backend-de-patches-proceduralmesh.md`.
5. `Docs/KNOWN_ISSUES.md`: lo abierto, con severidad.

## Qué hay

| Sistema | Código | Mapa / prueba |
|---|---|---|
| Direcciones, hash estable, constructor con faldones | `Planet/Patches` | `Patches.*` |
| Selector quadtree lineal, balanceado, determinista | `Planet/LOD` | `LOD.*`, `SelectReference` como referencia |
| Workers, cancelación, revisiones | `Planet/Streaming` | `Streaming.*` |
| Gestor de patches con relevo sin agujeros | `AstraeonPlanetPatchManager` | `PatchManager.*` |
| Backend ProceduralMesh | `AstraeonPlanetProceduralPatchBackend` | ADR 0006 |
| Anillo de colisión | `Planet/Collision` | `Collision.*`, `TL_13_CollisionRing` |
| Marco local (cambio de origen del mundo) | `AstraeonLocalFrameSubsystem` | `TL_14_FrameTransition` |
| Entidades, estado mutable, save v3 | `Planet/State`, `Persistence/AstraeonSaveMigration` | `Entities.*`, `State.*`, `SaveGame.V3*`, smoke `State` |
| Tránsito sobre la esfera | `Planet/Surface/AstraeonPlanetTraversal` | `Terrain.Connectivity`, `Terrain.TraversalDetectsWalls` |
| Laboratorio de LOD con vuelo libre | runtime + personaje | `TL_12_PatchLOD`, smokes `PatchLOD` y `Observer` |

Todos los mapas se generan con `Scripts/Editor/*.py` y se cocinan por `Config/DefaultGame.ini`.

## Cómo verificar

Ver el bloque de comandos de `Docs/evidencia/PUERTA_FASE_2.md`. `RunPlanetChecks.ps1` falla ante
cualquier `ensure` del motor.

## Lo que las pruebas encontraron, y conviene no repetir

- **Selector cuadrático** (19 ms por llamada, p99 33,6 ms). Lo encontró Insights, no una prueba
  unitaria: medir con el perfil antes de dar por buena una estructura de datos.
- **Un mapa de laboratorio tiene que funcionar con Play.** `TL_12` sólo funcionaba por línea de
  comandos y el propietario lo abrió en el editor: el personaje cayó a través del planeta.
- **Un rayo puede pasar por la costura de dos mallas de colisión**; una cápsula no.
- **La primera medida de jitter midió la captura de pantalla.** Una métrica tiene que tener en
  cuenta la duración real de cada frame.
- **El cambio de origen dejaba viejos los datos de luces en GPU Scene** (un `ensure` por sesión).
  Que un smoke "pase" no alcanza: hay que leer el log completo.
- **`RecordCreatureDeath` no tenía llamador**: el reloj de nido existía y nunca arrancaba.
- **La build empaquetada no cocinaba los laboratorios.** Sólo se había probado el mapa plano.

## Qué hereda la Fase 3

- Fauna planetaria apagada por defecto (`bSpawnFauna`); el mecanismo está probado.
- Region A sigue autorada en coordenadas planas y se ubica sobre la esfera por mapa exponencial
  (`FAstraeonPlanetRegionPlan`); la Fase 3 la autorea como región planetaria.
- Las estructuras colocadas del save siguen en coordenadas planas.
- Cuarentena: quedan sólo pruebas de la Fase 3; su puerta es dejar la tabla vacía.
- Pedido del propietario, no bloqueante: montañas más imponentes y variadas (`BACKLOG`).
- La criatura planetaria se apoya en la superficie analítica, no en el triángulo dibujado.
