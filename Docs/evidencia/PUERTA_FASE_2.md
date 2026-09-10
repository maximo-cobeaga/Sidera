# Puerta de la Fase 2 — evidencia

Fecha: 2026-09-10. Fase: **2 — Patches, LOD, precisión y estado mutable**.
Rama `main`, commits `ef56365` (P2.3-A) … cierre. Motor UE 5.7.4, Windows 11, i5-14400F / RTX 4060.

El propietario confirmó a mano `TL_11` y `TL_12` (P2.3) y autorizó cerrar el resto de la fase
sin más pruebas humanas; la evidencia de P2.4–P2.8 es automática, con capturas revisadas por el
agente. Una captura no reemplaza a una prueba ni una prueba a la inspección: hay de ambas.

**Prueba humana posterior al cierre, 2026-09-10:** el propietario jugó `TL_13_CollisionRing` y
`TL_14_FrameTransition` y los confirmó: *"estuvieron muy bien"*. Cubre a mano los criterios de
colisión bajo el jugador (anillo a 50 km) y de marco local en el tier Target.

## Criterio por criterio

| # | Criterio de la puerta | Evidencia | Estado |
|---|---|---|---|
| 1 | El tier Target de 500 km funciona sin que los patches de alta resolución crezcan linealmente con el radio | `TL_14` Target: caminata de 250 s con saltos, OK. **48 patches al nivel más fino a 500 km y 48 a 2.500 km** (el radio ×5, el detalle igual). `LOD.BalancedDeterministicEngineeringTiers`: hojas finas acotadas por el presupuesto en 10, 500 y 2.500 km. | **Cumple** |
| 2 | El stress test no produce jitter visible cerca del jugador | `TL_14` Stress (2.500 km): cámara quieta **0,0000 cm** de deriva y de paso; error de trayectoria p99 1,36 cm, igual que a 200 m (1,37); 8 cambios de marco sin discontinuidad; cardinales 26/26. A/B con y sin marco local: sin diferencia. | **Cumple** |
| 3 | No hay grietas visibles desde la ruta de prueba | Faldón medido (`LOD.SkirtsCoverCoarseNeighbourSeams`): la grieta más desfavorable es 0,855 del faldón (delta 2, 500 km). `TL_12`: 918 relevos y 754 auditorías de pantalla sin agujero ni solape. Capturas de esquina, arista y órbita revisadas. | **Cumple** |
| 4 | La colisión no desaparece bajo el jugador | **0 frames sin colisión** en todas las caminatas (`TL_11`, `TL_13`, `TL_14` Target y Stress) y en cardinales de los tres radios. `Collision.RingCoversCap`: 4.000 puntos del casquete cubiertos en centros, costuras y esquinas. | **Cumple** |
| 5 | Ir y volver regenera el mismo patch | `PatchManager.RoundTripRegeneratesSamePatch`: **1.104 patches reconstruidos, los 1.104 idénticos byte a byte**. `Planet.Entities.DeterministicPlacement`: mismas entidades en el mismo lugar al recargar. | **Cumple** |
| 6 | No hay hitches recurrentes superiores al presupuesto | Perfiles: `TL_11` p99 5,57 ms, `TL_12` vuelo p99 5,19 ms, `TL_14` Target p99 5,71 ms, frente a 16,67 ms de presupuesto. El único pico recurrente es la captura de pantalla del propio smoke, atribuido por tiempos en Insights. El selector cuadrático que daba p99 33,6 ms se corrigió en P2.3-C. | **Cumple** |
| 7 | Una criatura abatida sigue abatida tras descargar y recargar su patch | Smoke `State` en `TL_13`: abatida, 5 km lejos y de vuelta (su celda se descarga y recarga) **no revive**; guardado, estado borrado, carga: **sigue abatida** y el jugador a 0,0 cm de su lugar; vence el reloj de nido: vuelve. `Planet.State.DefeatSurvivesUnloadReloadAndSave`. | **Cumple** |
| 8 | `Terrain.Connectivity` y `Terrain.TraversalDetectsWalls` salen de cuarentena | Migradas a la esfera con todas sus aserciones: 7 seeds transitables al primer intento; el objetivo fuera de la región hace fallar la validación. | **Cumple** |

## Entregables de la fase

| Entregable | Dónde |
|---|---|
| `PlanetPatchManager`, `PlanetLODManager`, `PlanetStreamingManager` | `Planet/Patches`, `Planet/LOD`, `Planet/Streaming` |
| Backend de malla detrás de interfaz | `IAstraeonPlanetPatchMeshBackend`, `FAstraeonPlanetProceduralPatchBackend` |
| Workers, commit en game thread, cancelación, `BuildRevision` | streaming + gestor |
| Faldones y delta de LOD ≤ 1 | constructor + selector; faldón medido |
| Colisión sólo en el anillo cercano | `Planet/Collision` |
| `LocalReferenceFrameManager` y transición | `UAstraeonLocalFrameSubsystem` |
| `UAstraeonRuntimeStateManager` y save v2 → v3 | `Planet/State`, `Persistence/AstraeonSaveMigration` |
| `TL_12_PatchLOD`, `TL_13_CollisionRing`, `TL_14_FrameTransition` | `Content/Maps`, generados por script |
| Perfil en Unreal Insights | `TEST_REPORT` P2.3-C (vuelo `TL_12`) y P2.4 (caminata `TL_13`) |

## Cómo se reproduce

```powershell
.\Scripts\RunPlanetChecks.ps1 -Check Automation
.\Scripts\RunPlanetChecks.ps1 -Check Walk                      # TL_11
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals
.\Scripts\RunPlanetChecks.ps1 -Check PatchLOD -Profile         # TL_12
.\Scripts\RunPlanetChecks.ps1 -Check Observer
.\Scripts\RunPlanetChecks.ps1 -Check Walk -Map TL_13_CollisionRing
.\Scripts\RunPlanetChecks.ps1 -Check State                     # TL_13 con fauna
.\Scripts\RunPlanetChecks.ps1 -Check Walk -Map TL_14_FrameTransition -Profile -Extra '-AstraeonWalkStillSeconds=5'
.\Scripts\RunPlanetChecks.ps1 -Check Walk -Map TL_14_FrameTransition -RadiusCm 250000000 -Extra '-AstraeonWalkStillSeconds=5','-AstraeonFrameShiftCm=20000'
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals -Map TL_14_FrameTransition -RadiusCm 250000000
.\Scripts\RunPlanetChecks.ps1 -Check Critical                  # build plana, guardado v3
```

Cada corrida falla ante un `ensure` del motor, no sólo ante un resultado.
