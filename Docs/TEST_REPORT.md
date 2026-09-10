## 2026-09-10 — Fase 2, P2.8: build empaquetada y cierre de la puerta

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsFase2" -utf8output
```

`BUILD SUCCESSFUL`. **Defecto encontrado y corregido:** en la primera build, `TL_13` y `TL_14`
crasheaban con 0xC0000005, porque el mapa no estaba cocinado (`Failed to load package`) y el motor
abortaba al no poder entrar. Sin lista explícita sólo se cocinaba el mapa por defecto, y los
empaquetados anteriores sólo probaban ese mapa plano. `Config/DefaultGame.ini` ahora lista los
seis mapas que se cocinan.

Smokes sobre `Builds/WindowsFase2/Astraeon.exe`, ya con los mapas cocinados, sin ningún `ensure`:

| Smoke | Resultado |
|---|---|
| `State` en `TL_13` con fauna | OK: abatida, descargada, recargada, guardada y cargada, sigue abatida; repuebla al vencer |
| Caminata Stress 2.500 km, 60 s + 5 s quieto, cambios cada 200 m | OK: 3 cambios de marco, cámara quieta 0,0000 cm, error de trayectoria p99 1,18 cm |
| Vuelo de `TL_12` | OK: 747 auditorías sin agujero, 387 patches visibles como máximo |
| Recorrido crítico plano (`L_AstraeonBootstrap`) | OK: `Saved=true Loaded=true LoadedResolved=true` con save v3 |

Capturas del ejecutable revisadas: la caminata a 2.500 km muestra terreno continuo al horizonte
con 354 patches visibles y 6 de colisión; la órbita de `TL_12`, la esfera entera. Warnings nuevos
del proyecto: ninguno. Evidencia de los ocho criterios en
[evidencia/PUERTA_FASE_2.md](evidencia/PUERTA_FASE_2.md). Decisión del backend en
[ADR 0006](ADR/0006-backend-de-patches-proceduralmesh.md).

## 2026-09-10 — Fase 2, P2.7: `Terrain.Connectivity` y `TraversalDetectsWalls` salen de cuarentena

Migradas a la esfera en `Tests/AstraeonTerrainTraversalTests.cpp`, con validador nuevo en
`Planet/Surface/AstraeonPlanetTraversal.*`. Conservan los mismos nombres y **todas** sus aserciones;
cambia la superficie que recorren. El plan de Region A (objetivos, claros, exclusiones de
montaña, Ítaca y salida) se ubica sobre un planeta de 50 km con capa de montañas, por mapa
exponencial alrededor de un ancla. Las alturas salen del relieve radial de dos capas, con la
misma regla de claros y exclusiones que el campo plano, y la inundación va a la resolución real
de la malla más fina.

Como el relieve de un planeta es global y no se puede re-sembrar por región, lo que se resuelve
es dónde se ubica la región: hasta 8 anclas deterministas. Si ninguna sirve, se usa la variante
segura, que es la primera ancla cuya región no tiene capa de montaña.

| Aserción (plana → esférica) | Resultado |
|---|---|
| La validación mide a la resolución de la malla | igual al lado real del patch fino (1 %) |
| El desnivel admitido nunca baja del escalón | cumple |
| 7 seeds: declaran objetivos, región transitable, 0 inalcanzables, sin variante segura | las 7, al primer intento |
| Determinismo: misma seed, mismo lugar | cumple |
| La variante segura es transitable por sí misma | cumple |
| Con Ítaca aterrizada lejos, sigue transitable | cumple |
| Un objetivo fuera de la región hace fallar la validación y queda nombrado | cumple |

Peor desnivel alcanzado por seed: entre 8 y 580 cm, contra un límite de ~600 cm por pendiente
caminable. **96/96 Automation.** La tabla de cuarentena queda sin pruebas de la Fase 2; las que
restan son de la Fase 3.

## 2026-09-10 — Fase 2, P2.6: estado mutable, criatura abatida persistente y save v3

Implementación en `Planet/State/` (colocación determinista de entidades, `UAstraeonRuntimeStateManager`
y tipos de delta), `Persistence/AstraeonSaveMigration.*`, criatura planetaria en
`AstraeonCreatureActor`, streaming de entidades en el runtime y smoke `AstraeonPlanetStateSmoke`.

```powershell
.\Scripts\RunPlanetChecks.ps1 -Check Automation
.\Scripts\RunPlanetChecks.ps1 -Check State      # TL_13 con -AstraeonPlanetFauna
.\Scripts\RunPlanetChecks.ps1 -Check Critical   # recorrido plano con guardado y carga v3
```

Resultados: editor y juego Development **`Succeeded`**, sin warnings; **96/96 Automation**; `State`
OK; recorrido crítico plano OK (`Saved=true Loaded=true LoadedResolved=true`); regresión completa
de `TL_11`, `TL_12`, `TL_13`, `TL_14` Target y Stress, y observador en verde.

**Smoke de estado en `TL_13`**, el criterio de la puerta en un mundo vivo:

| Paso | Resultado |
|---|---|
| Presa más cercana a la aparición | `creature.planet_cube_sphere_lab.0.7.127.127.0`, a 167 m |
| Abatida por el camino del arma | delta registrado, reloj de repoblado 240 s |
| 5 km lejos y de vuelta (su celda se descarga y recarga) | **no revive**; 2 criaturas vecinas sí vuelven |
| Guardar, borrar el estado, cargar, restaurar | jugador a **0,0 cm** del lugar guardado; **sigue abatida** |
| Vence el reloj | vuelve a aparecer |

Pruebas nuevas:

- `Planet.Entities.DeterministicPlacement`: 4 criaturas en el banco de 200 m, 185 en 5 km a
  50 km de radio y 104 a 500 km. Mismo id y lugar al recargar, ninguna sobre montaña, otra seed
  otra fauna.
- `Planet.State.DefeatSurvivesUnloadReloadAndSave`: la derrota sobrevive a tres recargas de su
  celda y a serializar y deserializar el save; el índice espacial se reconstruye del save solo;
  el nido se repuebla cuando vence su reloj.
- `Persistence.SaveGame.V3PlanetaryLocation`: v2 → v3 con ubicación, rumbo, Ítaca y relojes de
  nido proyectados, y vuelta exacta al plano (0,01 cm); v1 → v3 conserva la regla v1 → v2; una
  versión futura se rechaza; sesión plana guardada y cargada por el slot real.
- `PatchManager.RoundTripRegeneratesSamePatch`: ida y vuelta dos veces sobre workers reales:
  **1.104 patches reconstruidos, los 1.104 idénticos byte a byte** a su primera construcción.

`Persistence.SaveGame.V1Migration` pedía "versión 2" porque era la vigente; ahora pide la versión
actual, con la misma exigencia.

**Defecto de P2.5 encontrado y corregido.** El log del smoke de estado traía un `ensure` del
motor: `GPUScene.cpp:820, Data for Id = N in GPU Scene Lights is stale`. Todas las corridas con
cambio de marco tenían exactamente uno y todas las demás cero: el cambio de origen mueve las luces
en la escena del renderer sin encargar su subida a GPU Scene. Con un sol no se ve; con luces
locales, iluminarían otro sitio. Se rehace el estado de render de las luces tras cada cambio, y
`RunPlanetChecks.ps1` ahora **falla ante cualquier `ensure`**. Todo lo que usa cambio de marco se
repitió con esa exigencia: vuelo de `TL_12`, caminata de `TL_13`, cardinales y caminata Stress
(57 cambios), caminata Target con perfil
([perf_baseline_20260910_171901.json](evidencia/perf_baseline_20260910_171901.json): 198,6 FPS,
p99 5,71 ms). Cero `ensure`.

## 2026-09-10 — Fase 2, P2.5: marco local y `TL_14_FrameTransition`

Implementación en `Planet/Coordinates/AstraeonLocalFrameSubsystem.*`, con `ApplyWorldOffset` en la
gravedad, el personaje y los smokes. `TL_14` es el tier Target (500 km) generado por
`CreatePlanetLabMap.py`; el Stress (2.500 km) es el mismo mapa con `-RadiusCm 250000000`. La
caminata suma una fase quieta (`-AstraeonWalkStillSeconds`) y dos medidas de jitter de la cámara,
relativas al planeta: el error contra la predicción de movimiento uniforme con los tiempos
reales de cada frame, y la deriva estando quieto.

```powershell
.\Scripts\RunPlanetChecks.ps1 -Check LabMap -Map TL_14_FrameTransition
.\Scripts\RunPlanetChecks.ps1 -Check Walk -Map TL_14_FrameTransition -Profile -Extra '-AstraeonWalkStillSeconds=5'
.\Scripts\RunPlanetChecks.ps1 -Check Walk -Map TL_14_FrameTransition -RadiusCm 250000000 -Extra '-AstraeonWalkStillSeconds=5','-AstraeonFrameShiftCm=20000'
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals -Map TL_14_FrameTransition -RadiusCm 250000000
# A/B, 60 s sin saltos, con -Extra '-AstraeonNoFrameShift' o '-AstraeonFrameShiftCm=20000'
```

Caminatas completas de 250 s con saltos, fase quieta de 5 s:

| | `TL_11` 200 m | `TL_13` 50 km | `TL_14` Target 500 km | `TL_14` Stress 2.500 km |
|---|---|---|---|---|
| Recorrido / saltos / Idle | 141.216 / 62 / 0 | 141.171 / 62 / 0 | 141.183 / 62 / 0 | 141.365 / 62 / 0 |
| Frames sin colisión | 0 | 0 | 0 | 0 |
| Cambios de marco (umbral) | 0 (5 km) | 1 (5 km) | 1 (5 km) | **8 (200 m)** |
| Cámara quieta: deriva / paso | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| Error de trayectoria p99 / máx. | 1,37 / 3,2 cm | 1,43 / 4,4 cm | 1,23 / 3,7 cm | 1,36 / 4,0 cm |
| Patches visibles al nivel más fino | — | — | **48** (nivel 13) | **48** (nivel 15) |

Cardinales 26/26 a 2.500 km: cada sitio queda a miles de kilómetros del anterior y fuerza un
cambio de marco; 100 patches de colisión, 0 frames sin suelo.

**A/B del marco local**, 60 s sin saltos:

| | Con cambios | Sin cambios |
|---|---|---|
| 500 km: error p99 / quieto | 1,15 cm / 0 | 1,43 cm / 0 |
| 2.500 km: error p50 / p99 / máx. / quieto | 0,40 / 1,66 / 4,5 cm / 0 | 0,29 / 1,20 / 3,5 cm / 0 |

**No hay jitter medible ni a 2.500 km, con o sin marco local.** Las coordenadas dobles de UE5
(LWC) ya cubren la precisión de física y cámara a esa escala. El marco local tampoco introduce
discontinuidad: el peor error nunca cae junto a un cambio. Se mantiene activo con umbral de 5 km
como acotación de las coordenadas absolutas cerca del jugador (`DECISIONS`), no porque haya
mejorado estas cifras.

Perfil Target: [perf_baseline_20260910_162338.json](evidencia/perf_baseline_20260910_162338.json),
197,9 FPS medios, p99 5,81 ms; el único hitch es la captura de pantalla.

Fallo de medición encontrado y corregido: la primera medida de jitter daba 218 cm siempre a los
30,4 s. Era la captura de pantalla: ese frame dura ~400 ms y el movimiento se limita a un paso
máximo. Se corrigió la medida (tiempos reales de frame y enfriamiento tras un frame irregular);
los hitches ya los mide el perfil.

Regresión: `TL_11`, `TL_13`, `TL_12` (vuelo orbital, con cambios de marco) y `Observer` en
verde; **92/92 Automation**; editor y juego Development sin warnings.

## 2026-09-10 — Fase 2, P2.4: anillo de colisión y `TL_13_CollisionRing`

Implementación en `Planet/Collision/AstraeonPlanetCollisionRing.*` y `AstraeonPlanetRuntime.*`;
mapas `TL_13_CollisionRing` (50 km, se camina, aparición en una esquina del cubo) y
`TL_14_FrameTransition` generados por `Scripts/Editor/CreatePlanetLabMap.py`. Salen el puente
de colisión de P2.3, el doble búfer de Fase 1 y `-AstraeonPlanetLegacyFaces`.

```powershell
.\Scripts\RunPlanetChecks.ps1 -Check LabMap -Map TL_13_CollisionRing
.\Scripts\RunPlanetChecks.ps1 -Check Automation
.\Scripts\RunPlanetChecks.ps1 -Check Walk -Map TL_13_CollisionRing
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals -Map TL_13_CollisionRing
.\Scripts\RunPlanetChecks.ps1 -Check Walk
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals
.\Scripts\RunPlanetChecks.ps1 -Check PatchLOD
.\Scripts\RunPlanetChecks.ps1 -Check Observer
```

Resultados: editor y juego Development **`Succeeded`**, sin warnings; **92/92 Automation**. Todos
los smokes OK.

| Caminata de 250 s | `TL_11` (200 m) | `TL_13` (50 km, desde una esquina) |
|---|---|---|
| Recorrido / saltos | 141.214 cm / 62 | 141.170 cm / 62 |
| Frames en Idle caminando | 0 | 0 |
| Frames sin colisión bajo el jugador | **0** | **0** |
| Patches de colisión construidos | **20** (antes 434 reconstrucciones) | 45 |
| De emergencia en el hilo de juego | 1 (arranque) | 1 (arranque) |
| Suelo visible ≠ pisado | 0 | 0 de 58.329 |

Cardinales 26/26 en los dos mapas, incluidas las 8 esquinas del planeta de 50 km.

Pruebas nuevas: `Collision.MatchesRenderedSurface` —en los cuatro radios de ingeniería, cada
triángulo de colisión es el triángulo dibujado, sin faldones— y `Collision.RingCoversCap` —4.000
puntos del casquete de 40 m, en centros de cara, costuras y esquinas, todos con colisión; como
máximo 4 patches en el anillo y 12 en el de conservación, sin importar el radio—. Reemplazan a
`Patches.CollisionBridgeMatchesFinestPatches`, cuyo puente ya no existe.

Insights, caminata de 120 s en `TL_13`: `Astraeon_PlanetCollision_Commit` 24 llamadas, **1,84 ms**
de media y 2,02 de máximo (el cocinado síncrono); `Astraeon_PlanetCollision_Update` 0,013 ms
de media. Se mantiene el cocinado síncrono: garantiza suelo en el instante del commit y pasa 0,2
veces por segundo.

Fallo encontrado y corregido en el camino: los cardinales no encontraban triángulo en la esquina
(−1, 1, −1) con 7 patches de colisión cargados. El rayo pasaba justo por el vértice que comparten
tres mallas distintas; se reemplazó por un barrido de esfera de 5 cm (`KNOWN_ISSUES`).

## 2026-09-10 — Fase 2, P2.3-D: prueba humana y observador de `TL_12`

Prueba humana: **`TL_11` confirmada por el propietario** ("anda ok"). **`TL_12` falló**: con
Play el personaje caía a través del planeta (`KNOWN_ISSUES`). Corregido con el modo observador
y verificado:

```powershell
.\Scripts\RunPlanetChecks.ps1 -Check Observer
.\Scripts\RunPlanetChecks.ps1 -Check PatchLOD
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals
.\Scripts\RunPlanetChecks.ps1 -Check Walk
.\Scripts\RunPlanetChecks.ps1 -Check Automation
```

Resultados: editor y juego Development **`Succeeded`**, sin warnings; **Observer
`RESULTADO=OK`** (97 m de avance, 49 m de subida, altura mínima 200 cm); vuelo de `TL_12` OK con
las mismas cifras que en C; cardinales 26/26; caminata de 250 s OK (141.216 cm, 62 saltos, 0
frames en Idle); **91/91 Automation**.

**Re-mirada humana de `TL_12`: confirmada por el propietario** ("anduvo correctamente"), con el
vuelo de observador. Única observación, estética y no bloqueante: las montañas se leen como
lomas. Queda en `BACKLOG`. **P2.3 cerrado.**

## 2026-09-10 — Fase 2, P2.3-C: `TL_12_PatchLOD`, faldón medido y selector lineal

Mapa `TL_12_PatchLOD` generado por `Scripts/Editor/CreatePatchLODTestMap.py`: 50 km de radio,
sin colisión cercana (`bNearCollision=false`). Smoke `AstraeonPlanetPatchLODSmoke`: cámara
scripteada, 185 s. A 20 m del suelo cruza la esquina +X/+Y/+Z, sube a 300 km, baja y sigue a
20 m la arista +X/+Z, y termina 10 s quieta. El pawn queda congelado y oculto.

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' AstraeonEditor Win64 Development 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' Astraeon Win64 Development 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex
.\Scripts\RunPlanetChecks.ps1 -Check Automation
.\Scripts\RunPlanetChecks.ps1 -Check PatchLODMap
.\Scripts\RunPlanetChecks.ps1 -Check PatchLOD -Profile -Trace
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals
.\Scripts\RunPlanetChecks.ps1 -Check Walk -Profile
```

Resultados: editor y juego Development **`Succeeded`**, sin warnings; **91/91 Automation**;
mapa `PASS` validado al recargar; **`TL_12` `RESULTADO=OK`**; cardinales 26/26; caminata de
250 s en verde (141.216 cm, 62 saltos, 0 frames en Idle, 0 de 52.495 con suelo desalineado).

**Vuelo en `TL_12`**, corrida final:

| Medida | Valor |
|---|---|
| Primera cobertura | 1,12 s, 384 patches |
| Auditorías de pantalla (una por relevo) | 754, **ningún agujero ni solape** |
| Pedidos / confirmados / relevos | 2.679 / 2.679 / 918; 0 fallos, 0 obsoletos |
| Patches visibles máx. / componentes máx. | 393 / 397 (tope 1.024) |
| Delta de LOD 2 en pantalla | 0,10 % de los frames |
| Bajo 100 m con el patch más fino debajo | **100 %** (27.072 frames) |
| Asentamiento con la cámara quieta | sí |

**Faldón medido** (`LOD.SkirtsCoverCoarseNeighbourSeams`). Mayor grieta entre el vértice fino
y la cuerda gruesa, dividida por el faldón que la tapa, en seis caras, una costura y un borde
interior, 1.024 celdas por línea y todos los niveles que el selector produce:

| Radio | Delta 1 | Delta 2 |
|---|---|---|
| 200 m | 0,005 | — |
| 50 km | 0,646 | 0,746 |
| 500 km | 0,777 | 0,855 |

Todo por debajo de 1: el faldón cubre también el delta 2 de los relevos. Ahora la prueba lo
exige para los dos deltas. Es un muestreo, no un recorrido exhaustivo de todas las celdas.

**El perfil encontró un defecto y se corrigió.** Primera corrida: p99 **33,61 ms**. Unreal
Insights (`TimingInsights.ExportTimerStatistics`, hilo de juego):

| Temporizador | Antes: llamadas, media, máx. | Después |
|---|---|---|
| `Astraeon_PlanetLOD_Select` | 1.819, **19,3 ms**, 34,7 ms | 1.823, **1,12 ms**, 2,57 ms |
| `AstraeonPlanetRuntime` (total del vuelo) | 37,0 s | 4,1 s |
| `Astraeon_PlanetPatches_Update` | 0,02 ms, 1,2 ms | 0,02 ms, 0,7 ms |
| `Astraeon_PlanetPatches_Commit` | 0,08 ms, 0,5 ms | 0,08 ms, 0,3 ms |

Causa: por cada subdivisión, el selector reordenaba todas las hojas, recalculaba el error de
cada una y rebalanceaba el árbol entero. Ahora usa cola de prioridad y balanceo incremental, con
el resultado probado idéntico en 624 vistas (`LOD.FastSelectionMatchesReference`: 0,26 contra
3,43 ms por llamada de media).

| Perfil del vuelo | FPS medio | p99 | Hitches > 50 ms |
|---|---|---|---|
| [Antes](evidencia/perf_baseline_20260910_113607.json) | 195,6 | 33,61 ms | 3 |
| [Después, con traza](evidencia/perf_baseline_20260910_114754.json) | 228,0 | 5,22 ms | 3 |
| [Final](evidencia/perf_baseline_20260910_120014.json) | 227,8 | 5,19 ms | 3 |

**Los 3 hitches son las capturas de pantalla.** En la traza, los frames de más de 50 ms caen a
38,2, 108,4 y 158,4 s: separados 70,3 y 50,0 s, igual que las capturas a los 30, 100 y 150 s de
la ruta. Explica también el pico aislado de 400 ms de todos los baselines anteriores: la caminata
saca una captura y tiene un hitch. Queda así el trabajo de patches en el hilo de juego: media
0,096 ms y máximo 4,38 ms, en los frames con selección (una cada 0,1 s).

Regresión de `TL_11` tras cambiar el observador del LOD de pawn a cámara:
[perfil](evidencia/perf_baseline_20260910_115327.json) 206,9 FPS, p99 5,57 ms (línea base
Fase 1: 209,3 y 5,56). La caída del 3 % registrada en B no se repite.

Capturas inspeccionadas: `PatchLOD_Corner`, `PatchLOD_Orbit` y `PatchLOD_Edge`. Terreno
continuo hasta el horizonte en la esquina y en la arista, esfera entera sin huecos desde órbita.
Traza de Insights: `Saved/Profiling/Planet_PatchLOD.utrace`, 545 MB, fuera del repositorio.

## 2026-09-10 — Fase 2, P2.3-B: `TL_11` renderiza por patches

Implementación en `Planet/Patches/AstraeonPlanetProceduralPatchBackend.*` y
`Planet/AstraeonPlanetRuntime.*`; guardián nuevo en `AstraeonPlanetWalkSmoke.cpp`; prueba nueva
`Patches.CollisionBridgeMatchesFinestPatches`. `RunPlanetChecks.ps1` acepta `-LegacyFaces`.

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' AstraeonEditor Win64 Development 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' Astraeon Win64 Development 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex
.\Scripts\RunPlanetChecks.ps1 -Check Automation
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals
.\Scripts\RunPlanetChecks.ps1 -Check Walk -Profile
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals -LegacyFaces
```

Resultados: editor y juego Development **`Succeeded`**, sin warnings; **89/89 Automation**.
La prueba del puente confirma, en las seis caras, que la colisión a rejilla 64 es triángulo por
triángulo y en el mismo orden de vértices la de los patches LOD 1 (el más fino en Lab).

- **Cardinals PASS 26/26** en modo patches. Primera cobertura: 24 patches en el frame 13;
  0 fallos, 0 obsoletos; **suelo visible distinto del pisado: 0 de 21.531 frames**.
- **Cardinals PASS 26/26 con `-LegacyFaces`**: el respaldo funciona (`mode=legacy_faces`).
- **Caminata 250 s `RESULTADO=OK`**: 141.215 cm (antes 141.224), 62 saltos, 0 frames en Idle
  caminando, racha de caída 1,03 s, **0 de 51.600 frames con suelo visible distinto del
  pisado**. 434 reconstrucciones de colisión, las mismas que antes: el radio y la cadencia en
  metros no cambiaron.

Perfil: [perf_baseline_20260910_110731.json](evidencia/perf_baseline_20260910_110731.json),
**203,8 FPS medios** (antes 209,3), p99 **5,81 ms** (antes 5,56), memoria 2.274→2.295 MB, el
mismo pico aislado de 400 ms que ya tenían los dos baselines anteriores. La diferencia es de un
3 % y no se atribuye todavía; el perfil de patches con relevos es el de `TL_12`.

Captura: se compararon `PlanetCardinalLab.png` en modo patches y con `-LegacyFaces` en el mismo
sitio. Son la misma imagen salvo el relieve más fino; los bloques de tono son el damero del
material y aparecen en las dos. Sin huecos ni grietas visibles.

Lo que esto **no** prueba: en Lab la selección son 24 patches LOD 1 y no cambia al caminar (un
relevo en toda la sesión). Relevos en movimiento, faldones y conteo transitorio se miden en `TL_12`.

## 2026-09-10 — Fase 2, P2.3-A: gestor de patches con relevo sin agujeros

Implementación en `Planet/Patches/AstraeonPlanetPatchManager.*`, interfaz de cola en
`Planet/Streaming/AstraeonPlanetStreamingManager.h`, `ValidatePartition` en el selector y
`Tests/AstraeonPlanetPatchManagerTests.cpp`. **El runtime no cambia en este incremento.**

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' AstraeonEditor Win64 Development 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' Astraeon Win64 Development 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex
.\Scripts\RunPlanetChecks.ps1 -Check Automation
```

Resultados: editor y juego Development **`Succeeded`**, sin warnings; **88/88 Automation**. El
guardián exige ahora al menos 88 y las tres pruebas nuevas por nombre:

- `PatchManager.HoleFreeSplitAndMerge`: la primera cobertura aparece entera o no aparece; en un
  split el padre sigue en pantalla hasta que el cuarto hijo está listo; en un merge los hijos
  siguen hasta que el padre está listo; un cambio de objetivo a mitad de relevo cancela lo
  pendiente, retira lo oculto y el padre nunca desaparece.
- `PatchManager.RejectsStaleFailedAndInvalid`: objetivo desbalanceado, rejilla inválida y cuerpo
  ajeno rechazados; tope de subidas por llamada; revisión falsificada descartada sin consumir
  tope; resultado de una dirección liberada nunca llega al backend aunque la cola lo entregue;
  un build fallido deja al padre en pantalla, se cuenta una vez y no se reintenta cada frame.
- `PatchManager.DeterministicRouteWithWorkers`: selector real sobre una ruta que cruza una arista
  y una esquina, más rápida que los builds a propósito. Partición verificada tras **cada**
  llamada. Dos corridas: **2.428 operaciones de backend idénticas**, 708 commits, 108 relevos.
  La misma ruta sobre el pool real de workers asienta en cada parada y el registro del
  streaming nunca supera los 2 trabajos pendientes.

Dato medido: durante un relevo llegaron a verse **462 patches** a la vez con un objetivo de hasta
384, porque conviven el conjunto saliente y el entrante. Se registra en `KNOWN_ISSUES`.

## 2026-09-10 — Fase 2, P2.1/P2.2/P2.3 parcial: patches, workers y LOD

Implementación identificable en `Planet/Patches/`, `Planet/Streaming/`, `Planet/LOD/` y
`Tests/AstraeonPlanetPatchTests.cpp`/`AstraeonPlanetLODTests.cpp`; integración del constructor
mediante `BuildFace`.

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' AstraeonEditor Win64 Development 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' Astraeon Win64 Development 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex
.\Scripts\RunPlanetChecks.ps1 -Check Automation
.\Scripts\RunPlanetChecks.ps1 -Check Walk -Profile
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals
```

Resultados: editor y target de juego Development **`Succeeded`**, sin warnings de compilación;
**85/85 Automation**, cero fallos; caminata de 250 s `RESULTADO=OK`; **Cardinals PASS en las
26 direcciones**, salto y aterrizaje comprobados. Se repitió Automation para verificar el
guardián actualizado: ahora exige todas las pruebas descubiertas y las diez nuevas por nombre.
No aparecieron warnings nuevos de proyecto. Persisten los dos avisos del motor sobre iconos
VisionOS en los smokes y `EditorPerf`/sección `Compile` en Automation, presentes en el baseline.

Las diez pruebas nuevas cubren jerarquía y bordes de dirección, vector fijo de hash,
separación de canales/versiones/seed, presupuesto de malla, faldones radiales separados de
colisión, precisión de conversión local float a los tres tiers, coincidencia de normales y
posiciones entre caras y muestras de LOD consecutivo, entradas inválidas, cancelación,
presión de cola, revisión obsoleta tras descarga y determinismo con orden de solicitud invertido.
Las 75 anteriores siguen pasando; ninguna sale de cuarentena en esta iteración.

Regresión de `TL_11`: **141.224 cm**, 62 saltos, 0,0% desalineado, 0,0% fuera de altitud,
racha máxima de caída 1,04 s, **cero frames en Idle mientras camina**. Continúan 434
reconstrucciones de colisión en 250 s; esa deuda heredada se paga en P2.4.

Perfil: [perf_baseline_20260910_021547.json](evidencia/perf_baseline_20260910_021547.json),
1920×1080, 10 s de calentamiento y 30 s de muestra, **209,3 FPS medios**, p99 **5,56 ms**,
memoria 2.256→2.282 MB. Un pico aislado de 400 ms; no se atribuye causalmente ni se oculta.
El baseline previo tenía también un pico de 400 ms. Esta es evidencia de regresión del mapa
de seis caras, **no** perfil de LOD ni de workers materializando en gameplay.

Captura inspeccionada: `Saved/Screenshots/WindowsEditor/PlanetWalkLab.png`, generada a las
02:15:37: terreno y horizonte presentes, orientación coherente y herramienta visible. No
reemplaza la futura inspección visual de `TL_12`/`TL_13`/`TL_14`.

Fallos encontrados y corregidos durante esta iteración: copia implícita del manager de
futures al exportar la clase (C2280); assert de `TArray::Add` por argumento dentro del mismo
array al construir faldones. Se resolvieron con propietario no copiable y copias locales,
respectivamente, manteniendo todas las pruebas. Un intento de build Game fue bloqueado al
rotar el log de UBT en AppData; se reejecutó con la autorización de sandbox requerida.

El selector quadtree/LOD ya está probado, pero la Fase 2 **no cierra**: faltan su integración
con el runtime y backend en `TL_12`, anillo de
colisión, marcos locales en vivo, deltas y save v3, recuperación de dos pruebas, Insights y
build empaquetada. Estado detallado en `PHASE_STATUS.md` y `BACKLOG.md` P2.3–P2.8.

## Estado actualizado 2026-09-10 — `Terrain.Relief` recuperada sobre el contrato radial

```powershell
.\Scripts\RunPlanetChecks.ps1 -Check Automation   # PASS, 75 pruebas
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals    # PASS
.\Scripts\RunPlanetChecks.ps1 -Check Walk         # PASS, RESULTADO=OK
```

`Astraeon.WorldGen.Terrain.Relief` corre ahora contra `FAstraeonPlanetSurface` por dirección
planetaria global. Conserva sus ocho aserciones y suma una: `WorstGroundStepCm > 1.0`, para que el
límite de escalón no pueda aprobarse con un mundo liso.

Medido sobre el tier Lab de 10 km, seis seeds, rejilla de 61 × 61 direcciones:

| | |
|---|---|
| Peor escalón del suelo | **7,2 cm** (límite 45) |
| Montaña más alta | **5.100 cm** |
| Suelo por debajo del nivel del mar | ninguno |
| Claro de Ítaca | nivela a la altura del suelo local; a 3× el radio, terreno intacto |

Tres pruebas verdes de la Fase 1 se ajustaron al relieve de dos capas, sin bajar ningún listón:
`Planet.Height.Determinism` actualiza su valor fijo por el cambio de `GeneratorVersion` a 3;
`Planet.Surface.ContinuityAndInvalidInput` pasa a exigirle la continuidad a la **capa de suelo**
—que es la que se camina— y añade que un cuerpo de 200 m no recibe capa de montañas mientras uno
de 10 km sí; `Planet.Topology.MeshAtEngineeringTiers` sigue igual.

Rendimiento tras el nuevo relieve: `perf_baseline_20260910_013511.json`, 205,3 FPS medios, p99 de
**6,99 ms**, 1 hitch de arranque. Sin coste medible frente a los 7,24 ms de antes del cambio.

## Estado actualizado 2026-09-10 — corte de locomoción: control, causa y arreglo

```powershell
.\Scripts\RunPlanetChecks.ps1 -Check Automation   # PASS, 75 pruebas
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals    # PASS
.\Scripts\RunPlanetChecks.ps1 -Check Walk         # PASS, RESULTADO=OK
```

Instrumentación nueva del smoke: `-AstraeonNoJump` (control: caminar sin saltar), `-AstraeonSprint`
(reproduce la carrera), reparto de clips por frames, transiciones entre clips, reconstrucciones de
colisión y frames en `Idle` mientras camina.

Caminata de 40 s en línea recta, sin saltos, radio 200 m:

| | antes | después |
|---|---|---|
| cambios de clip | 107 (2,67/s) | **1** (el arranque) |
| transiciones `Walk_F->Idle` | 53 | **0** |
| frames en `Walk_F` | 99,0 % | **100,0 %** |
| recorrido | 16.922 cm | **20.934 cm** (+24 %) |
| reconstrucciones de colisión | 55 | 67 *(más distancia recorrida)* |
| en caída | 0,0 % | 0,0 % |

Corriendo (`-AstraeonSprint`): `Run_F` 99,2 %, dos transiciones, ambas de arranque.

Caminata larga de 250 s con saltos: 141.247 cm, 62 saltos, desalineado 0,0 %, fuera de altitud
0,0 %, racha en el aire máxima 1,06 s, **125 cambios de clip = 62 saltos × 2 + 1 arranque**, y
0 frames en `Idle` mientras camina. `RESULTADO=OK`.

El guardián se calibró contra datos medidos: el defecto daba ~1,0 % de frames en `Idle` caminando
y el arreglo da 0,00 %; el umbral está en 0,5 %.

### Evidencia de cierre de fase (`AGENTS.md` §15.1)

- **Build**: `Build.bat AstraeonEditor Win64 Development -Rebuild`, recompilación completa del
  módulo en 57,7 s. `Result: Succeeded`, **cero warnings y cero errores**.
- **Perfil de rendimiento re-medido durante la caminata**, 1920×1080, post-arreglo:
  `perf_baseline_20260910_010156.json` — 203,1 FPS medios, mediana 4,67 ms, **p99 7,24 ms**,
  1 hitch de arranque/streaming, memoria 2.258 → 2.290 MB.
- **Regresión medida y aceptada**: el p99 pasó de **5,41 ms a 7,24 ms** frente al baseline del
  2026-09-09. La causa es más trabajo de colisión por segundo, y en parte porque el personaje
  ahora recorre un 24 % más de distancia en el mismo tiempo. Sigue **muy dentro del presupuesto
  de `AGENTS.md` §8**: el objetivo del percentil 1 % es 45 FPS y 7,24 ms son 138 FPS, con más de
  3× de margen. El hitch no es recurrente.
- **Capturas**: `Saved/Screenshots/WindowsEditor/PlanetWalkLab.png` y `PlanetCardinalLab.png`,
  regeneradas por las corridas posteriores al arreglo.

## Estado actualizado 2026-09-10 — animación del protagonista corregida e integrada

El `.blend` canónico fue guardado; el FBX pulido se importó y guardó en
`/Game/Astraeon/Characters/Player/Optimized_Polished`.

```powershell
.\Scripts\RunCharacterChecks.ps1 -Check Automation       # PASS
.\Scripts\RunCharacterChecks.ps1 -Check Critical         # PASS
.\Scripts\RunCharacterChecks.ps1 -Check Visual           # PASS
.\Scripts\RunCharacterChecks.ps1 -Check Package         # PASS
.\Scripts\RunCharacterChecks.ps1 -Check PackagedCritical # PASS
.\Scripts\RunCharacterChecks.ps1 -Check PackagedVisual   # PASS
```

Evidencia: `ContentPipeline/reports/polished_character_animation_import.json`,
`graphics/characters/main_player/docs/locomotion_polish_20260910.json` y
`Saved/Logs/Character_*.log`. El ciclo `Walk_F` mantiene 37 frames, 1,2 s y costura exacta
entre frame 1 y 37. La pose A usa `upperarm X = -1,08 rad`; C++ añadió histeresis y continuidad
de fase al cambiar entre ciclos. La medición de patinaje y la prueba humana siguen pendientes.
Detalle de implementación y procedimiento de prueba: [ANIMACION_PROTAGONISTA_CORRECCION_20260910.md](ANIMACION_PROTAGONISTA_CORRECCION_20260910.md).

Corrección posterior: se detectó que `AN_HandsFP_*` seguía congelado en la pose del escáner.
Se creó `/Game/Astraeon/Art/Blockouts/Human/PolishedFP` y se conectó al runtime. Validación
de movimiento real: Walk 0,069 rad, Run 0,105 rad, Jump 0,062 rad y Land 0,037 rad de variación
angular en los brazos. `Package` y `PackagedVisual`: PASS.

Renders nuevos revisados: `Saved/BlenderRecovery/walk_arm_silhouette_final_f01.png`,
`walk_arm_silhouette_final_f10.png`, `walk_arm_silhouette_final_f20.png` y
`walk_arm_silhouette_final_f37.png`; frames 1 y 37 conservan la misma pose de cierre.

## 2026-09-10 — Animación del protagonista: Bridge/Blender

Fuente activa: `CHR_Astraeon_Player.blend`, escena `CHR_Astraeon_Player_Work`, 30 FPS.
Se revisaron visualmente renders de `Idle`, `Walk_F`, `Run_F` y de las tres fases del salto. Se
modificaron `Idle`, `Walk_F/B/L/R`, `Run_F/B/L/R` y `Jump_Start/Loop/Land`: brazos separados,
codos compactos y loops/rangos conservados. Auditoría estructural: las curvas modificadas
siguen en Bézier. No se consumieron créditos. El `.blend` quedó guardado y el FBX fue reexportado
e importado; la medición de patinaje queda pendiente.

Evidencia: `graphics/characters/main_player/docs/locomotion_polish_20260910.json`.

## 2026-09-10 — Fase 1: núcleo esférico, locomoción y rendimiento

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' AstraeonEditor Win64 Development Astraeon.uproject -WaitMutex -NoHotReloadFromIDE
.\Scripts\RunPlanetChecks.ps1 -Check Automation
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals
.\Scripts\RunPlanetChecks.ps1 -Check Walk -Seconds 270 -Profile
```

Resultado: build `Succeeded`; **75/75 Automation**; `Cardinals PASS`; `Walk PASS` con
131.483 cm recorridos, 67 saltos, 0,0% de frames desalineados, 0,0% fuera de altitud y
captura `PlanetWalkLab` a 1920×1080. Perfil: 211,5 FPS medios, mediana 4,62 ms, p99 5,41 ms,
peor frame 400 ms, 1 hitch >50 ms y memoria 2.267→2.306 MB. El hitch queda atribuido al
arranque/carga y debe volver a medirse en Fase 2 con streaming de patches.

La puerta de Fase 1 sigue abierta por las dos pruebas de terreno en cuarentena
(`Terrain.Relief`, `Terrain.SurfaceContract`) y por la confirmación humana pendiente.

## 2026-09-09 — Fase 1: contrato de definición y coordenadas

```powershell
Build.bat AstraeonEditor Win64 Development Astraeon.uproject -WaitMutex -NoHotReloadFromIDE
.\Scripts\RunCharacterChecks.ps1 -Check Automation
```

Resultado: build `Succeeded`; **73 tests verdes, 0 fallos**. Se verificaron definición válida,
round-trip dirección ↔ cara/UV y muestras de borde/esquina. Persisten tres avisos conocidos
`LogAutomationTest: Error: Condition failed` del motor, sin resultado fallido de Astraeon.

## 2026-09-09 — Vuelta completa a la esfera tras la primera prueba manual

```powershell
.\Scripts\RunCharacterChecks.ps1 -Check Automation   # 69 verdes, 0 fallos
.\Scripts\RunCharacterChecks.ps1 -Check Critical     # recorrido plano completo, en verde

# Caminata planetaria automática (la prueba nueva)
UnrealEditor-Cmd.exe Astraeon.uproject /Game/Maps/TL_10_RadialGravity -game `
    -unattended -nosplash -RenderOffscreen -AstraeonSmokePlanetWalk -AstraeonWalkSeconds=180
```

**`-AstraeonSmokePlanetWalk`** existe porque los tres defectos que encontró la prueba manual son
de comportamiento sostenido, y ninguna prueba unitaria de matemática los veía: el marco estaba
bien y el personaje terminaba boca abajo igual, porque quien lo rotaba era otro sistema.

Camina de frente sin tocar la mirada, salta a mitad de camino, y cuenta frames en vez de abortar
al primero malo — distinguir "una vez" de "todo el rato" es exactamente lo que hacía falta.

| Progreso | Arco desde el polo |
|---|---:|
| t=15 s | 58,2° |
| t=45 s | 99,2° |
| t=90 s | **170,1°** (antípoda cruzado) |
| t=135 s | 119,0° (volviendo por el otro lado) |
| t=165 s | 71,7° |

| Métrica | Resultado | Umbral |
|---|---:|---:|
| Recorrido | **1.004 m** de 1.257 m de circunferencia | > 100 m |
| Frames desalineados | **0,0 %** (peor cos 0,994 ≈ 6°) | < 5 % |
| Frames en caída | **2,0 %** — el salto | < 25 % |
| Frames fuera de altitud | **0,0 %** | < 5 % |
| Altitud | 98,2–98,4 cm toda la vuelta | −50 a 400 cm |
| Velocidad | 553 constante, sin frenadas | — |

Dos pruebas nuevas del marco, además de las siete anteriores:

| Prueba | Qué fija |
|---|---|
| `Astraeon.Planet.Frame.TransportKeepsHeading` | 360 pasos de medio grado por un meridiano: el rumbo se conserva y el frente sigue tangente **en cada paso**, no sólo al final |
| `Astraeon.Planet.Frame.YawIsAroundLocalUp` | En el antípoda, girar 90° gira respecto al suelo del jugador y no al Z global; cuatro cuartos de vuelta no acumulan deriva |

**Sin cubrir por automatización**: si la cámara se *siente* bien. Que no ruede ni se invierta está
medido; que acompañe al jugador sin marearlo sigue necesitando una partida.

## 2026-09-09 — Spike de gravedad radial: 65 verdes, 0 fallos

```powershell
# Compilar
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' `
    AstraeonEditor Win64 Development -Project="...\Astraeon.uproject" -WaitMutex
# → Result: Succeeded

.\Scripts\RunCharacterChecks.ps1 -Check Automation
# → Exit=0 · Success=65 Fail=0
```

| | |
|---|---|
| Pruebas | **65** (58 previas + 7 nuevas), todas en verde |
| Ninguna prueba previa tocada | Las 58 pasan sin modificación: el spike no alteró el mundo plano |
| Cuarentena | Vacía en ejercicio: el mapa plano sigue en el repositorio |

Pruebas nuevas, todas de `FAstraeonPlanetFrame` y sin dependencia de mundo:

| Prueba | Qué fija |
|---|---|
| `Astraeon.Planet.Frame.UpIsRadial` | Arriba es la dirección radial en siete direcciones, no el Z global |
| `Astraeon.Planet.Frame.AntipodeIsInverted` | Dos antípodas tienen arribas opuestos y ambos marcos son válidos |
| `Astraeon.Planet.Frame.AlignmentIsOrthonormal` | Barrido de 437 puntos sobre la esfera (19 polares × 23 azimutales, primos entre sí para no repetir meridianos) sin un solo marco degenerado |
| `Astraeon.Planet.Frame.PoleDegeneracyIsHandled` | Frente paralelo al arriba —el caso que rompe una alineación ingenua— produce marco válido y sin NaN |
| `Astraeon.Planet.Frame.TangentProjection` | Lo proyectado es perpendicular al arriba; un vector vertical se anula |
| `Astraeon.Planet.Frame.AltitudeAtTargetScale` | 180 cm sigue siendo medible sobre un radio de 500 km (con float de 32 bits no lo sería) |
| `Astraeon.Planet.Frame.AlignmentIsProgressive` | Un giro de 180° no se resuelve en un frame, y converge en dos segundos |

**Generación del test level**, verificada cargando el mapa y listando sus actores:

```powershell
UnrealEditor-Cmd.exe Astraeon.uproject -unattended -nosplash -RenderOffscreen `
    -ExecutePythonScript=Scripts\Editor\CreateRadialGravityTestMap.py
# → Mundo activo verificado: /Game/Maps/TL_10_RadialGravity
# → 8 actores · harness radio=20000 centro=(0,0,0) · PlayerStart a 20120 cm del centro
```

**No usar `-nullrhi` con ese script** — ver `KNOWN_ISSUES.md`. Las pruebas sí lo usan y pasan: el
fallo está en la ruta de spawn del editor, no en la del juego.

**Sin cubrir por automatización, y sigue abierto:** la cámara en el antípoda. Requiere partida
humana; es el criterio que falta de la puerta de la Fase 0.

## 2026-09-07 — Bridge en vivo y proxy del protagonista

- `get_host_status`: `blr:true`; `bl_get_scene_summary` y `bl_execute` inspeccionan
  instancia real Blender 5.2.1 LTS, METRIC, scale 1. Scene original con Cube/Camera/Light.
- Test de cuatro vértices y seis keys: creación/modificación/retirada, inventario igual PASS.
- `Tools/Blender/main_character_proxy.py` ejecutado por `bl_execute` en instancia viva.
  Nueva escena aislada con 23 mallas proxy y cámaras frontal/perfil, 30 FPS.
- Primera inspección detectó suelas inclinadas y 1.8676 m por orientar las secciones de
  botas según su centro longitudinal; se corrigieron las secciones horizontales y las normales.
- Revisión v02: 1.830000281 m, min Z -5.82e-11 m, escalas 1, cero aristas no manifold,
  volumen positivo por malla. Vistas frontal/perfil renderizadas y vistas por el agente.
  Original Scene conserva Cube/Camera/Light. No certifica continuidad anatómica entre piezas.
- Checkpoints y fuente proxy v02 guardados en `graphics/characters/main_player/blender/`;
  reporte `docs/bridge_proxy_validation_20260907.json`, previews `references/*_v02.png`.
- `bl_list_models(type=3d)`: catálogo recibido. No se expone estimador ni generador de motion
  dedicado. No se enviaron jobs. Rig, skin, expresiones, texturas, animación y export UE
  **NOT VERIFIED**; no se compila C++ ni se ejecuta smoke al no cambiar gameplay/Content.

## 2026-09-07 — Reinspección Bridge: servidor accesible, Blender desconectado

- Reanudación solicitada: apertura autorizada con `Start-Process -FilePath 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' -WindowStyle Normal`, exit 0.
  PID 11784, `Responding:true`; consulta posterior `get_host_status`: `blr:false`.
  Abrir la aplicación no conectó automáticamente el panel; escena aún NOT VERIFIED.
- Inventario ejecutable: herramientas `mcp__higgsfield_bridge__bl_*` presentes,
  incluyendo `bl_execute`, generación 3D e `bl_import_motion`. No se expone `bl_generate_motion`.
- `get_host_status({})`, dos consultas: `blr:false`, `Blender: not connected`.
- `bl_get_skill({name:"blender-scene"})`: contenido recibido.
- `bl_get_scene_summary({})`: `isError:true`, mensaje exacto:
  `Blender is not connected. Open the Higgsfield panel in Blender and press Connect in "Supercomputer Connection".`
- `ConvertTo-Json -InputObject @(Get-Process | Where-Object { $_.ProcessName -like '*blender*' } | Select-Object Id,ProcessName,MainWindowTitle)`:
  exit 0, lista vacía. Una variante previa con `-AsArray` falló porque el PowerShell
  disponible no admite ese parámetro; se corrigió sin instalar dependencias.
- Inspección visual de `ContentPipeline/Generated/HumanoidBlockout/Preview_Idle.png`:
  blockout previo, casco sin rostro visible; no cumple el personaje solicitado.
- Tres vías: consulta de host, consulta de escena y procesos locales. La prueba aislada
  anterior no se repitió ni se presenta como prueba del Bridge. Ninguna mutación Blender.
- Nueva producción, rig, skinning, IK, animaciones, materiales, QA y export Unreal:
  **NOT VERIFIED**. Sin generación ni gasto de créditos. Sólo actualización documental;
  compilación/Automation/smoke no ejecutados porque no cambió código ni contenido del juego.

## 2026-09-07 — Preflight del nuevo personaje, no QA de producción

- Higgsfield `scene_builder_3d_list_projects`: `ok:true`, cero proyectos.
- Endpoint `https://bridge.higgsfield.ai/mcp`, POST initialize: error de conexión en
  sandbox; fuera del sandbox HTTP 401 `Unauthorized`. No se enviaron credenciales.
- Blender local detectado PID 3624; consulta elevada de puertos de escucha sin entradas
  para ese PID. Archivo/escena activa mediante Bridge: **NOT VERIFIED**.
- Prueba aislada ejecutada con exit 0:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background 'C:\Users\MAXIMO\Desktop\Astraeon\ContentPipeline\Generated\HumanoidBlockout\Humanoid_Blockout_Source.blend' --disable-autoexec --python-exit-code 1 --python Tools/Blender/inspect_main_character_preflight.py -- --report graphics/characters/main_player/docs/preflight_20260907.json
```

Blender 5.2.1 LTS; tres objetos y siete acciones previos. Malla temporal creada/modificada
y retirada, acción temporal con keyframes retirada; inventario igual antes/después y
SHA256 fuente conservado. Reporte de salida exclusivo: para repetir usar otra ruta JSON.
Inspección visual de `Preview_Idle.png`: blockout, no rostro visible ni calidad AAA.
No se crearon assets nuevos ni se verificó Character Animation remoto. Rig nuevo,
deformación, animaciones nuevas y exportación UE: **NOT VERIFIED**. Sin cambios C++,
compilación ni smoke nuevo. Detalle en `graphics/characters/main_player/docs/MAIN_CHARACTER_PIPELINE.md`.

## 2026-09-06 — A02: sesión y persistencia de Khepri / Region A

- Las nuevas partidas usan el PlanetProfile y el layout fijo. `ContentSeed` se registra como
  variación secundaria, mientras que recursos, POIs y ambiente permanecen authored.
- SaveGame v2 persiste `PlanetProfileId`, `RegionProfileId` y `ContentSeed`. La prueba nueva
  `Astraeon.Persistence.SaveGame.V1Migration` verifica que una partida v1 conserva layout,
  inventario y coordenadas, y que su siguiente snapshot es v2.
- `AstraeonEditor Win64 Development`: compilación exitosa, 9,74 s.
- Automation: **55/55** exitosos, 0 fallos. Log:
  `Saved/Logs/RegionAProfileA02Tests.log`.
- Smoke crítico editor-game: `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true
  Loaded=true LoadedResolved=true Seed=13579`. Log:
  `Saved/Logs/RegionAProfileCriticalPathSmoke.log`.

## 2026-09-06 — A01: perfiles y layout fijo de Region A

- Añadidos `FAstraeonPlanetProfile`, `FAstraeonRegionProfile`, seeds secundarias y el registro
  C++ `UAstraeonWorldProfiles`. Region A es `planet_khepri` /
  `region_first_signal_basin`; geografía conceptual, recursos y POIs principales son estables.
- Diseño de rutas, zonas reservadas y restricciones PCG en
  `Docs/DISENO_RECORRIDO_REGIONAL.md`.
- `AstraeonEditor Win64 Development`: compilación exitosa, 6,86 s.
- Automation: **54/54** exitosos, exit code 0. La prueba nueva
  `Astraeon.WorldGen.Profiles.RegionA` valida perfil resuelto por ID, límites de 500 × 500 m,
  IDs únicos y posiciones fijas dentro de la región. Evidencia:
  `Saved/Logs/RegionAProfileTests.log`.
- No se modificaron sesión, savegame, materialización ni terreno actual: eso corresponde a A02.

## 2026-09-06 — Revisión de arquitectura de mundo fija

- Directiva registrada en `MVP_WORLD_ARCHITECTURE.md`, auditoría en
  `WORLD_ARCHITECTURE_AUDIT.md` y decisión en ADR 0003.
- Se clasificaron los sistemas actuales como KEEP, ADAPT o DEFER. No hay REMOVE: no se
  eliminaron archivos, no se cambió gameplay y no corresponde ejecutar pruebas Unreal para
  este ajuste de dirección. Las pruebas vigentes continúan siendo las de T00/T01 parcial.

## 2026-09-06 — T00 y T01 parcial: contrato y prototipo de terreno continuo

### Cambios verificados

- `FAstraeonTerrainSurfaceContext` y `AAstraeonTerrainField::SampleSurface` describen la
  superficie final sin actores: seed efectiva, centro, claros, exclusiones de montaña y
  plataforma de Ítaca producen altura en cm, normal y validez.
- `AAstraeonTerrainSurfacePrototype` construye una superficie continua aislada, de 640 × 640 m
  y 400 cm entre muestras: 25.921 vértices y 51.200 triángulos. El campo de cubos no fue
  sustituido ni se cambió gameplay.
- Se habilitó `ProceduralMeshComponent`, plugin oficial ya incluido con UE 5.7; decisión y
  límites en `ADR/0002-continuous-terrain-prototype.md`.

### Comandos y resultados

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' AstraeonEditor Win64 Development 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex -NoHotReloadFromIDE
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -ExecCmds='Automation RunTests Astraeon; Quit' -unattended -nullrhi -nosplash -nop4 -testexit='Automation Test Queue Empty' -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\TerrainT01Tests.log'
```

- Build editor: exitosa, 32,38 s.
- Automation: **53/53** exitosos, exit code 0. La prueba nueva
  `Astraeon.WorldGen.Terrain.SurfaceContract` valida diez seeds, determinismo, normal,
  plataforma, límites y topología.
- Primer intento de Automation dentro del sandbox: falló antes de iniciar tests porque Zen/DDC
  no tenía nodo escribible. Repetido fuera del sandbox con autorización, exitoso. Log de fallo:
  `Saved/Logs/TerrainT00BaselineTests.log`; resultado válido:
  `Saved/Logs/TerrainT01Tests.log`.

### Pendiente

Falta spawn runtime de la malla, trazos/barridos contra su colisión, coste de cocción, memoria,
FPS y prueba renderizada. T01 permanece en verificación y no autoriza T02.

## 2026-09-06 — Revisión documental del plan de terreno

- Creado `PLAN_TERRENO_REGIONAL.md` con T00–T06 pendientes, método, ubicaciones y aceptación.
- Verificados archivo de destino, enlaces al plan desde backlog/estado/decisiones y
  correspondencia de los siete IDs con sus fichas. `Test-Path` confirmó el plan.
- `git diff --check -- Docs/BACKLOG.md Docs/DEVELOPMENT_STATE.md Docs/DECISIONS.md`:
  sin errores de whitespace; avisos de normalización LF/CRLF del repositorio.
- No se ejecutaron tests Unreal, compilación ni empaquetado: el cambio es exclusivamente
  documental. Este registro no añade evidencia de funcionamiento del terreno propuesto.

## 2026-09-06 — Exterior de Ítaca, estaciones y escotilla con hoja

### Comandos

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/Blender/generators/ship_blockout.py
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -run=pythonscript -script='C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\PrepareShipBlockoutAssets.py' -unattended -nullrhi -nosplash -nop4 -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\ShipPackages.log'
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' AstraeonEditor Win64 Development -Project='C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -ExecCmds="Automation RunTests Astraeon;Quit" -unattended -nullrhi -nosplash -nop4 -testexit="Automation Test Queue Empty" -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\ShipTests.log'
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' /Game/Maps/L_AstraeonBootstrap -game -unattended -nullrhi -nosplash -nop4 -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\ShipCriticalPathSmoke.log'
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsItacaExterior" -utf8output
& "C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsItacaExterior\Astraeon.exe" /Game/Maps/L_AstraeonBootstrap -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -nullrhi -unattended -nosplash -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\PackagedItacaExteriorSmoke.log"
```

### Resultado

- Blender: 17/17 controles (10 negativos, 7 repeticiones de determinismo), 7 assets validados
  contra el contrato estático completo y 7 roundtrips FBX. Montaje verificado dentro del
  volumen de estudio: 13,0 × 10,0 × 5,87 m.
- Importación y guardado: 7 paquetes en `Content/Astraeon/Art/Blockouts/Itaca/`, dimensiones
  exactas en centímetros y pivote al suelo. Exit 0, commandlet 0 errores / 0 warnings.
- `AstraeonEditor Win64 Development`: compilación exitosa, sin warnings.
- Automation Tests `Astraeon.*`: **51 encontrados, 51 exitosos**, exit 0.
- Smoke crítico editor-game: `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- BuildCookRun Win64 Development: `BUILD SUCCESSFUL`, archive en `Builds/WindowsItacaExterior`.
- **Smoke crítico packaged**: `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.

Se archivó a una carpeta nueva a propósito: en corridas anteriores el archive falló porque el
ejecutable viejo seguía abierto y Windows bloqueaba el archivo.

### Cobertura nueva

`Astraeon.Art.Itaca.ExteriorAndStations`: cuenta los módulos del exterior y verifica que
ninguno tenga colisión ni escala distinta de 1, que no sobreviva ningún cubo del motor en la
nave, que el casco contenga la estancia de 8 × 6 × 2,8 m y entre en el volumen de estudio, que
las dos estaciones traigan malla apoyada en el suelo con caja bloqueante, y que la hoja de la
escotilla empiece cerrada, bloquee el paso, ignore el canal de visibilidad y deje de bloquear
al abrirse.

### No verificado

Nada de esto sustituye una sesión jugada: encuadre, ritmo, legibilidad del exterior desde la
cámara de vuelo y sensación de la escotilla al abrirse siguen pendientes de revisión humana.

## 2026-09-06 — Humano y manos animadas Q1

Comandos completos: `HUMANOID_ART.md`. Blender 5.2.1 LTS y Unreal 5.7.4 existentes.

| Verificación | Resultado |
| --- | --- |
| Fuente y skin | PASS: 57 huesos, cuerpo 1,80 m / 9.256 tris, manos 4.336 tris |
| Controles negativos | 4/4 rechazos esperados: escala, pesos, vértice sin peso, anclaje |
| Reproducción | Dos construcciones de geometría con firma semántica idéntica |
| Animaciones | 7/7; 261 fotogramas, dos mallas; ciclos y contacto de botas verificados |
| FBX roundtrip | 9/9, jerarquía, dimensiones, pesos y animaciones |
| Importación UE | exit 0; 2 SkeletalMesh, 7 AnimSequence sobre un Skeleton, 57 huesos; 180 cm |
| Poses UE | Movimiento y cierre de ciclos evaluados; commandlet 0 errores / 0 warnings |
| Manifiesto | PASS: nueve FBX con hashes coincidentes entre fuente e importación |
| Video Blender | exit 0; 120 fotogramas, 30 FPS, 4 s, 512×576; dos ciclos de marcha y tres de carrera |

Evidencia en `ContentPipeline/reports/humanoid_*.json` y
`Saved/Logs/HumanoidImportVerified.log`. Cinco renders Blender revisados para proporción,
locomoción y flexión del guante; no certifican arte final ni gameplay.

Incidencias corregidas: limpieza de objetos ocultos en roundtrip; elevación de pelvis
en eje local correcto y contacto con suelo; orden variable de polígonos de UV spheres
normalizado; contenedor de armature eliminado del intercambio mediante la convención
soportada por el importador UE. Primer intento Unreal dentro del sandbox falló antes
del script por acceso a Zen/DDC; repetición autorizada fuera del sandbox aprobada.

No se modificaron C++ ni mapas ni se guardaron paquetes Unreal. No se recompiló ni
reempaquetó el juego; no se atribuyen a este lote la suite/smoke jugable de entregas
anteriores. Los cambios ajenos preexistentes se conservaron.

## 2026-09-06 — Siete blockouts regionales integrados

Comandos reproducibles: `REGION_ART_INTEGRATION.md`. Ejecutados con Blender 5.2.1 LTS y
Unreal 5.7.4, sin actualizar dependencias.

| Verificación | Resultado |
| --- | --- |
| Generación Blender | exit 0; 15 controles, 7 meshes válidos, 7 roundtrips FBX |
| Importación Unreal | exit 0; 7 assets persistidos, cm/pivote correctos; 0 errores/0 warnings del commandlet |
| `AstraeonEditor Win64 Development` | Succeeded, sin warnings nuevos de C++ |
| `RunRegionArtChecks.ps1 -Check Automation` | exit 0; 46/46 tests, incluido `Astraeon.Art.Region.PresentationPreservesGameplay` |
| `-Check Visual` | exit 0; siete meshes en runtime, siete escaneos, tres recolecciones y dos rechazos sin taladro; siete capturas 1920×1080 revisadas |
| `-Check Critical` | exit 0; Deployed/Scanned/Crafted/Resolved/Saved/Loaded/LoadedResolved=true |
| `-Check Package` | exit 0; BUILD SUCCESSFUL; `Builds/WindowsRegionArt` |
| `-Check PackagedVisual` | exit 0; mismo smoke de siete nodos con assets cocinados |
| `-Check PackagedCritical` | exit 0; todos los campos críticos true |
| Manifiesto complementario | siete assets con hashes de fuente/importación coincidentes |

Evidencia: `ContentPipeline/reports/region_*.json`, `Saved/Logs/RegionArt_*` y
`Saved/Screenshots/RegionArt/`. Package y smokes packaged sin líneas Warning/Error/Fatal
en los logs revisados. Persisten los tres `LogAutomationTest: Error: Condition failed`
de bootstrap NullRHI previos a los tests; el runner no confunde exit 0 con suite aprobada.

Incidencias corregidas durante la iteración:

- El validador rechazó tapas n-gon: trianguladas antes de exportar.
- Primer test usaba actores sin registrar: cambiado a mundo transitorio para verificar
  transform real. Se corrigió la doble inicialización de ese mundo, que causaba un fatal
  de `WorldSettings`; `CreateWorld` ya lo inicializa en UE5.7. Suite final completa en verde.
- El layout contiene una instrucción de spawn de criatura sin asset regional: se excluye
  explícitamente de los siete objetos de presentación, sin desactivar pruebas existentes.
- Smoke gráfico reprodujo sobrescritura del aviso del taladro; corregida y exigida en
  ambas vetas por input real. Primeras capturas 888×500: se fija resolución 1920×1080 en
  el smoke y se verifica en las capturas finales; no es una medición de FPS.
- El sandbox impedía escribir cachés Unreal: repetición autorizada fuera del aislamiento.
  `python` era un alias de Microsoft Store; se usa el Python ya incluido en Blender.

Límites: sin checkout limpio ni nuevo smoke humano; no se certifica calidad Q2, rendimiento,
animación o cierre global del MVP. Cambios ajenos preexistentes preservados, sin commit mixto.

## 2026-09-05 — Pipeline de arte: escala e Ítaca Q1

Alcance: herramientas offline y cinco static blockouts originales. No se modificó código C++ ni mapas en esta entrega; no se recompiló/reempaquetó el juego y no se cierra un hito MVP.

### Comandos y resultados

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/Blender/generators/itaca_blockout.py
```

- Blender observado: 5.2.1 LTS. Corrida final exit 0, `ASTRAEON_ART_VALIDATION: PASS`.
- 15 controles aprobados: 10 rechazos esperados de geometría/estado inválido y 5 repeticiones deterministas.
- 5 meshes validados, normales hacia fuera y shells cerradas, presupuesto/pivote/unidades/materiales correctos; 5 FBX separados con roundtrip métrico correcto.
- Triángulos: humano 96, suelo 12, pared 12, marco 28, consola 24 (172 total). No son presupuestos de escena final.
- Fuente y preview `.blend`, 5 FBX y PNG disponibles en `ContentPipeline/Generated/ItacaBlockout/`. Preview inspeccionada: silueta humana, proporciones de consola/panel y hueco del marco legibles; calidad blockout, sin detalle final.
- Evidencia máquina: `ContentPipeline/reports/itaca_blockout_validation.json`, hashes de configuración, geometría y FBX.

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -run=pythonscript -script='C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\ValidateArtBlockoutImport.py' -unattended -nullrhi -nosplash -nop4 -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonArtImport.log'
```

- Primer intento en sandbox: Zen no encontró un directorio de datos válido y falló el lanzamiento repetidamente; detenido antes de ejecutar el script. Se repitió con permiso fuera del sandbox, sin cambiar dependencias.
- Corrida final UE 5.7.4: exit 0, `ASTRAEON_ART_IMPORT: PASS`, `Success - 0 error(s), 0 warning(s)`; revisión de log sin errores/warnings en esa corrida.
- 5 StaticMesh importados en memoria, escala uniforme 1 y conversión de unidades. Bounds cm: humano 70×30×180, suelo 200×200×12, pared 200×12×280, marco 180×24×250, consola 80×60×120 (tolerancia 0,1 cm).
- No guarda paquetes ni mapa. No certifica orientación visual en Unreal, colisión, materiales, iluminación, animación o gameplay.
- Reporte: `ContentPipeline/reports/itaca_unreal_import.json`, vinculado por hash a cada FBX final. Log completo local en `Saved/Logs/AstraeonArtImport.log`.

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/Blender/build_manifest.py
git diff --check
```

- Manifiesto: exit 0, `ART_MANIFEST: PASS 36 entries; 5 generated blockouts`; 31 entradas son familias planificadas. Verifica IDs únicos, referencias, ausencia de ciclos y evidencia actual de config/FBX. Las cinco importaciones se registran como transitorias; `unreal_integrated=false`.
- Python estándar: alias `python.exe` inaccesible en sandbox y fuera de él informa que no hay Python instalado (alias Store). Se reutiliza el Python de Blender sin instalar nada.
- Revisión de whitespace: sin errores; Git avisa normalización LF→CRLF según configuración local.
- Se preservó el trabajo ajeno preexistente en C++ y documentos. No se creó commit global del árbol mezclado; las fuentes del pipeline quedan listas para revisión/commit separado.

### Próxima verificación

Montaje de kit en ensayo aislado, colisión por piezas del marco con paso libre, cápsula real y cámara; después integración de Ítaca y smoke manual de ARGOS/escotilla. El fallo manual previo sigue abierto. No confundir importación dimensional aprobada con un AC-03 jugable aprobado.

## 2026-09-05 — Resultado manual negativo: ESCOTILLA no responde

### Evidencia del usuario

- Después de Rebuild6, el usuario reportó estar frente a `ESCOTILLA` y presionar `E` sin que ocurra nada visible.
- Conclusión: el bug sigue abierto para smoke manual. La evidencia automática de `-nullrhi` no es suficiente para cerrar la interacción de hatch.

### Acción recomendada

- Ver `Docs/HANDOFF_ESCOTILLA_INTERACCION.md`.
- El próximo cambio debe empezar con instrumentación visible/log de `AAstraeonPlayerCharacter::Interact()` para distinguir si falla el input, la detección de marker o la condición de despliegue.

## 2026-09-05 — Rebuild6: ESCOTILLA con interacción más robusta

### Comandos ejecutados

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" AstraeonEditor Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -unattended -nullrhi -nosplash -nop4 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonAutomation.log" -ExecCmds="Automation RunTests Astraeon; Quit" -TestExit="Automation Test Queue Empty"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" Astraeon Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
$env:MSYS_NO_PATHCONV=1; & "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" /Game/Maps/L_AstraeonBootstrap -game -unattended -nullrhi -nosplash -nop4 -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonCriticalPathSmoke.log"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment" -utf8output
$env:MSYS_NO_PATHCONV=1; & "C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment\Astraeon.exe" /Game/Maps/L_AstraeonBootstrap -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -nullrhi -unattended -nosplash -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonPackagedCriticalPathSmoke.log"
```

### Resultado

- `AstraeonEditor`: exitoso.
- Automation Tests `Astraeon.*`: 35 encontrados, 35 exitosos, exit code 0.
- `Astraeon`: exitoso.
- Smoke crítico editor-game: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- Primer BuildCookRun archive: el cook/stage completó, pero el archive no pudo sobrescribir `Builds/WindowsDevelopment/Astraeon.exe` porque el juego estaba abierto y Windows bloqueó el archivo.
- Segundo BuildCookRun Win64 Development: `BUILD SUCCESSFUL` con archive completo.
- Smoke crítico packaged: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.

### Cobertura nueva

- Fallback de interacción ampliado a 450 cm, con selección por cercanía a la mira y fallback de marcador muy cercano.
- ESCOTILLA temporal con escala Z `1.2` para que sea un blanco más visible y más fácil de impactar.
- Smoke crítico colocado más lejos (`360 cm`) y mirando horizontalmente por encima de la escotilla para cubrir un caso más parecido al fallo manual.

### Observaciones

- Para probar builds manuales, cerrar siempre `Astraeon.exe` antes de reempaquetar. Si el exe queda abierto, Windows puede dejar el package anterior en `Builds/WindowsDevelopment` aunque el cook compile código nuevo.

## 2026-09-05 — Rebuild5: interacción `E` tolerante y pruebas sincronizadas

### Comandos ejecutados

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" AstraeonEditor Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -unattended -nullrhi -nosplash -nop4 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonAutomation.log" -ExecCmds="Automation RunTests Astraeon; Quit" -TestExit="Automation Test Queue Empty"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" Astraeon Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
$env:MSYS_NO_PATHCONV=1; & "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" /Game/Maps/L_AstraeonBootstrap -game -unattended -nullrhi -nosplash -nop4 -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonCriticalPathSmoke.log"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment" -utf8output
$env:MSYS_NO_PATHCONV=1; & "C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment\Astraeon.exe" /Game/Maps/L_AstraeonBootstrap -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -nullrhi -unattended -nosplash -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonPackagedCriticalPathSmoke.log"
```

### Resultado

- `AstraeonEditor`: exitoso.
- Automation Tests `Astraeon.*`: 35 encontrados, 35 exitosos, exit code 0.
- `Astraeon`: exitoso.
- Smoke crítico editor-game: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- BuildCookRun Win64 Development: `BUILD SUCCESSFUL`.
- Smoke crítico packaged: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.

### Cobertura nueva

- `AAstraeonPlayerCharacter::Interact()` ahora usa fallback por proximidad de 180 cm si el trace directo no detecta un marcador.
- El smoke crítico de `SURFACE HATCH` ahora apunta horizontalmente por encima del hatch temporal, por lo que valida que el fallback cubra el caso manual de apuntado imperfecto.
- Las pruebas de HUD/hints/labels se sincronizaron con las cadenas españolas reales del build temporal.

### Observaciones

- La primera corrida de Automation Tests de esta sesión falló por expectativas obsoletas en inglés (`SURFACE HATCH`, `Seed`, `Hint`, etc.) frente a UI actual en español (`ESCOTILLA`, `Semilla`, `Pista`, etc.). Se corrigieron las pruebas; la corrida posterior pasó completa.
- Falta smoke manual visual sin `-nullrhi`: interacción real con `E`, bruma/rocas, save/close/open/continue y rendimiento.

## 2026-09-05 — Fix caída al usar SURFACE HATCH

### Comandos ejecutados

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" AstraeonEditor Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -unattended -nullrhi -nosplash -nop4 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonAutomation.log" -ExecCmds="Automation RunTests Astraeon; Quit" -TestExit="Automation Test Queue Empty"
$env:MSYS_NO_PATHCONV=1; & "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" /Game/Maps/L_AstraeonBootstrap -game -unattended -nullrhi -nosplash -nop4 -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonCriticalPathSmoke.log"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" Astraeon Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment" -utf8output
$env:MSYS_NO_PATHCONV=1; & "C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment\Astraeon.exe" /Game/Maps/L_AstraeonBootstrap -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -nullrhi -unattended -nosplash -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonPackagedCriticalPathSmoke.log"
```

### Resultado

- `AstraeonEditor`: exitoso.
- Automation Tests `Astraeon.*`: 35 encontrados, 35 exitosos, exit code 0.
- Smoke crítico editor-game: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- `Astraeon`: exitoso.
- BuildCookRun Win64 Development: `BUILD SUCCESSFUL`.
- Smoke crítico packaged: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.

### Cobertura nueva

- `SURFACE HATCH` ya no teleporta a una coordenada inline: usa `UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm()`.
- El teleport usa `ETeleportType::TeleportPhysics`, detiene movimiento inmediatamente, fuerza `MOVE_Walking` y revela mapa alrededor del destino.
- La materialización runtime crea una plataforma dedicada de despliegue bajo ese punto, además de la superficie regional amplia.
- El smoke crítico verifica que la interacción real con el hatch registre despliegue y deje al jugador en el punto seguro esperado.

## 2026-09-05 — Corrección visual inicial HUD/iluminación

### Comandos ejecutados

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" AstraeonEditor Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -unattended -nullrhi -nosplash -nop4 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonAutomation.log" -ExecCmds="Automation RunTests Astraeon; Quit" -TestExit="Automation Test Queue Empty"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" /Game/Maps/L_AstraeonBootstrap -game -unattended -nullrhi -nosplash -nop4 -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonCriticalPathSmoke.log"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" Astraeon Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment" -utf8output
$env:MSYS_NO_PATHCONV=1; & "C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment\Astraeon.exe" /Game/Maps/L_AstraeonBootstrap -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -nullrhi -unattended -nosplash -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonPackagedCriticalPathSmoke.log"
```

### Resultado

- `AstraeonEditor`: exitoso.
- Automation Tests `Astraeon.*`: 35 encontrados, 35 exitosos, exit code 0.
- Smoke crítico editor-game: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- `Astraeon`: exitoso.
- BuildCookRun Win64 Development: `BUILD SUCCESSFUL`.
- Smoke crítico packaged con `MSYS_NO_PATHCONV=1`: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.

### Cobertura nueva

- `AAstraeonGameModeBase::EnsureRuntimeLighting()` crea sol direccional y skylight runtime para evitar pantalla negra y dependencia de iluminación baked.
- `BeginPlay()` desactiva mensajes on-screen de Unreal para ocultar warnings debug como `Lighting needs to be rebuilt` durante el smoke manual.
- Feedback de gameplay ahora se muestra en el HUD propio mediante `UAstraeonGameInstance::LastFeedbackMessage`, en vez de `GEngine->AddOnScreenDebugMessage`.
- El HUD usa panel sombreado, fuente media, sombras y espaciado mayor para evitar solapamiento en la esquina superior izquierda.

### Observaciones

- Se reprodujo una corrida fallida del smoke packaged desde Git Bash sin `MSYS_NO_PATHCONV=1`: MSYS convirtió `/Game/...` a `C:/Program Files/Git/Game/...`. La repetición con `MSYS_NO_PATHCONV=1` pasó.
- Falta revalidación visual/manual del ejecutable sin `-nullrhi`.

## 2026-09-05 — Smoke usa interacción real con hatch

### Comandos ejecutados

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" AstraeonEditor Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -unattended -nullrhi -nosplash -nop4 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonAutomation.log" -ExecCmds="Automation RunTests Astraeon; Quit" -TestExit="Automation Test Queue Empty"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" /Game/Maps/L_AstraeonBootstrap -game -unattended -nullrhi -nosplash -nop4 -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonCriticalPathSmoke.log"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" Astraeon Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment" -utf8output
& "C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment\Astraeon.exe" -nullrhi -unattended -nosplash -log -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonPackagedCriticalPathSmoke.log"
```

### Resultado

- `AstraeonEditor`: exitoso.
- Automation Tests `Astraeon.*`: 35 encontrados, 35 exitosos, exit code 0.
- Smoke crítico editor-game: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- `Astraeon`: exitoso.
- BuildCookRun Win64 Development: `BUILD SUCCESSFUL`.
- Smoke crítico packaged: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`, exit code 0.

### Cobertura nueva

- El smoke crítico ya no registra el despliegue sólo por llamada directa de sesión: apunta al `SURFACE HATCH` runtime y ejecuta `AAstraeonPlayerCharacter::Interact()`.
- La creación de superficie runtime ahora registra warning explícito si falla la carga del mesh o el spawn del actor.

## 2026-09-05 — Ítaca/hatch runtime y superficie regional

### Comandos ejecutados

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" AstraeonEditor Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -unattended -nullrhi -nosplash -nop4 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonAutomation.log" -ExecCmds="Automation RunTests Astraeon; Quit" -TestExit="Automation Test Queue Empty"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -run=pythonscript -script="C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\SmokeBootstrapMap.py" -unattended -nullrhi -nosplash -nop4 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonMapSmokeCommandlet.log"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" /Game/Maps/L_AstraeonBootstrap -game -unattended -nullrhi -nosplash -nop4 -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonCriticalPathSmoke.log"
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" Astraeon Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment" -utf8output
& "C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment\Astraeon.exe" -nullrhi -unattended -nosplash -log -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonPackagedCriticalPathSmoke.log"
```

### Resultado

- `AstraeonEditor`: exitoso.
- Automation Tests `Astraeon.*`: 35 encontrados, 35 exitosos, exit code 0.
- Smoke de mapa: exitoso, 0 errores/0 advertencias.
- Smoke crítico editor-game: `AstraeonCriticalPathSmoke: Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- `Astraeon`: exitoso.
- BuildCookRun Win64 Development: `BUILD SUCCESSFUL`.
- Smoke crítico packaged: `AstraeonCriticalPathSmoke: Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`, exit code 0 en Git Bash.

### Cobertura nueva

- Especificación runtime de `itaca_argos_console` y `itaca_surface_hatch`.
- Label/color legible para `SURFACE HATCH`.
- Entrada de bitácora para despliegue controlado desde Ítaca.
- Hints de objetivo: ARGOS → SURFACE HATCH → escaneo ambiental.
- Superficie regional runtime amplia para que el recorrido manual no dependa del stub de 20 m del mapa base.

### Observaciones

- En Git Bash, los argumentos Unreal que empiezan con `/Game/...` deben ejecutarse con `MSYS_NO_PATHCONV=1`; sin eso se convierten erróneamente a `C:/Program Files/Git/Game/...`.
- Se intentó regenerar el mapa con `CreateBootstrapMap.py`; `EditorLevelLibrary.new_level` sobre un asset existente provocó crash. No se modificó el `.umap`; el avance quedó resuelto por materialización runtime en C++.

## 2026-09-05 — Build C++ actual

### Comandos ejecutados

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" AstraeonEditor Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" Astraeon Win64 Development -Project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE
```

### Resultado

- `AstraeonEditor`: exitoso.
- `Astraeon`: exitoso.

## 2026-09-05 — Automation Tests H0-H6 parcial

### Comando ejecutado

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -unattended -nullrhi -nosplash -nop4 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonAutomation.log" -ExecCmds="Automation RunTests Astraeon; Quit" -TestExit="Automation Test Queue Empty"
```

### Resultado

- Última corrida: exit code 0.
- Tests encontrados: 33.
- Tests exitosos: 33.
- La cola propia termina con `**** TEST COMPLETE. EXIT CODE: 0 ****`.

### Coberturas destacadas

- Determinismo ambiental y variación por seed.
- Invariantes ambientales y clasificación de respirabilidad.
- Generación determinista de región y límites de radio.
- Materialización de recursos/POIs.
- Identidad visual temporal de marcadores.
- Nueva partida con seed y entrada inicial de bitácora.
- Menú HUD C++ con seed editable e instrucciones.
- GameMode con PlayerController/HUD/Pawn correctos.
- HUD temporal con estado de sesión e hints de objetivo.
- Mapa revelable y persistencia de celdas.
- Save/load round-trip de seed, ambiente, región, mapa, inventario, objetivo y bitácora.
- Traje: oxígeno, daño ambiental, daño directo por amenaza y movilidad por gravedad.
- Mob: estados por distancia, perfil aplicable, patrulla/escala visual por estado.
- Escaneo de criatura a bitácora.
- Inventario y acumulación de recursos.
- Crafting de `signal_resonator`.
- Briefing ARGOS a bitácora.
- Resolución de fuente de señal y entrada final de bitácora.
- Functional critical path: briefing → scan → collect → craft → signal → save/load.

### Observaciones

- El log registra tres `LogAutomationTest: Error: Condition failed` durante arranque del editor, antes de la cola `Astraeon.*`.
- Esas líneas no pertenecen a tests del proyecto; la cola `Astraeon.*` pasa completa.

## 2026-09-05 — Smoke de mapa bootstrap

### Comando ejecutado

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -run=pythonscript -script="C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\SmokeBootstrapMap.py" -unattended -nullrhi -nosplash -nop4 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonMapSmokeCommandlet.log"
```

### Resultado

- Exit code: 0.
- `Content/Maps/L_AstraeonBootstrap.umap` carga correctamente.
- MapCheck: 0 errores, 0 advertencias.
- Actores requeridos presentes: `PlayerStart_ItacaBootstrap`, `Floor_DeterministicRegionStub`.

## 2026-09-05 — Critical-path smoke editor-game

### Comando ejecutado

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" /Game/Maps/L_AstraeonBootstrap -game -unattended -nullrhi -nosplash -nop4 -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonCriticalPathSmoke.log"
```

### Resultado

- Exit code: 0.
- Evidencia: `AstraeonCriticalPathSmoke: Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.

## 2026-09-05 — Package Development Win64

### Comando ejecutado

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment" -utf8output
```

### Resultado

- Última corrida: `BUILD SUCCESSFUL`.
- Cook: 435 packages cooked.
- Archive: `Builds/WindowsDevelopment`.

## 2026-09-05 — Critical-path smoke packaged

### Comando ejecutado

```powershell
& "C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDevelopment\Astraeon.exe" -nullrhi -unattended -nosplash -log -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonPackagedCriticalPathSmoke.log"
```

### Resultado

- El proceso no dejó exit code confiable en PowerShell, pero no quedó corriendo.
- Evidencia: `AstraeonCriticalPathSmoke: Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- No se observaron fatales en las líneas revisadas.

## Pendiente de verificación

- Smoke visual/manual del HUD, labels y controles.
- Save/close/open/continue verificado manualmente desde ejecutable interactivo.
- Medición básica de rendimiento.

## 2026-09-08 — Protagonista optimizado: import a Unreal y regresión

### Comandos ejecutados

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' '-ExecutePythonScript=C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\ValidateMainCharacterImport.py' -unattended -nosplash -nop4 -nullrhi -AstraeonPlayerOptimized '-abslog=C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\MainCharacterOptimized.log'
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' AstraeonEditor Win64 Development -Project='C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex -NoHotReloadFromIDE
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -ExecCmds="Automation RunTests Astraeon;Quit" -unattended -nullrhi -nosplash -nop4 -testexit="Automation Test Queue Empty" -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\PlayerBodyTests2.log'
```

### Resultados

| Prueba | Resultado |
|---|---|
| `ValidateMainCharacterImport -AstraeonPlayerOptimized` | `passed: true`, `saved_packages: true`; 183,00 cm, 4 LOD, 75 huesos, 45 clips, 3 morph targets, 4 materiales. Reporte: `ContentPipeline/reports/main_character_unreal_optimized.json` |
| `AstraeonEditor Win64 Development` | Succeeded, 13,00 s, sin warnings nuevos |
| `Automation RunTests Astraeon` | **55 éxitos, 0 fallos**, exit code 0 |

### Nota sobre una regresión detectada y revertida

Apuntar el cuerpo de sombra de `AstraeonPlayerCharacter` al protagonista nuevo compiló pero
hizo fallar `Astraeon.Art.Character.FirstPersonRigIsWired`:

```
Body and hands share one skeleton: The two values are not equal.
AstraeonFirstPersonRigTests.cpp(47)
```

La prueba tiene razón: las manos de primera persona están sobre `SKEL_Humanoid_A` (57
huesos) y el protagonista trae uno de 75. No se debilitó la prueba —AGENTS.md §7 lo
prohíbe—: se revirtió el cambio y se documentó la decisión pendiente en
`Docs/PENDIENTE_PROTAGONISTA.md`, P9. La tabla de arriba corresponde al estado revertido.

### Cuatro corridas hasta el verde

La validación de Unreal falló tres veces por contratos que la sesión anterior había escrito
sin llegar a ejecutar: `sys.path` sin la carpeta del script, `SkeletalMesh.set_material`
inexistente en la API de UE 5.7, y el destino a medio guardar de la corrida anterior.

## 2026-09-08 — Protagonista integrado como cuerpo de sombra

### Comandos ejecutados

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' AstraeonEditor Win64 Development -Project='C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex -NoHotReloadFromIDE
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -ExecCmds="Automation RunTests Astraeon;Quit" -unattended -nullrhi -nosplash -nop4 -testexit="Automation Test Queue Empty" -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\IntegrationTests.log'
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' /Game/Maps/L_AstraeonBootstrap -game -unattended -nullrhi -nosplash -nop4 -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\PlayerBodySmoke.log'
```

### Resultados

| Prueba | Resultado |
|---|---|
| `AstraeonEditor Win64 Development` | Succeeded, 2,95 s |
| `Automation RunTests Astraeon` | **55 éxitos, 0 fallos**, exit code 0 |
| Smoke recorrido crítico | `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`; 0 fatales |
| Cableado del CDO | malla `/Optimized/SK_Astraeon_Player`, 183,0 cm, 4 LOD, 3 morph targets, Z relativo −96, yaw −90, `bOwnerNoSee` true. Reporte: `ContentPipeline/reports/player_body_wiring.json` |

### Prueba sustituida, no debilitada

`Astraeon.Art.Character.FirstPersonRigIsWired` exigía que el cuerpo de sombra y las manos de
primera persona compartieran esqueleto. Esa igualdad no era el requisito: era una forma
indirecta de comprobar que los gestos resuelven sobre el esqueleto que anima las manos y que
la silueta lee a escala. Se sustituyó por esas comprobaciones directas —esqueleto con raíz
`root`, altura entre 170 y 195 cm, raíz apoyada en el suelo de la cápsula— y los nueve
gestos se siguen verificando contra el esqueleto de las manos, como antes.

El motivo de fondo, medido: en este rig **el eje que lleva el brazo al frente es Z**, y los
45 clips lo usan entre 0,13 y 0,46. En `TwoHand_Idle` las manos quedan a x = ±0,48 m.
Unificar los esqueletos habría obligado a re-autorizar las poses de brazo de los 45 clips.
Detalle en `Docs/PENDIENTE_PROTAGONISTA.md`, P9.

### Build empaquetada con el protagonista

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsProtagonista" -utf8output
& "C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsProtagonista\Astraeon.exe" /Game/Maps/L_AstraeonBootstrap -AstraeonAutoSmokeCriticalPath -AstraeonSeed=13579 -nullrhi -unattended -nosplash -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\PackagedProtagonistaSmoke.log"
```

| Prueba | Resultado |
|---|---|
| `BuildCookRun` Win64 Development | `BUILD SUCCESSFUL`, 49,92 s; archive en `Builds/WindowsProtagonista` |
| Smoke sobre el ejecutable | `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579` |

## 2026-09-08 — Vista en tercera persona (tecla V)

| Prueba | Resultado |
|---|---|
| `AstraeonEditor Win64 Development` | Succeeded |
| `Automation RunTests Astraeon` | **55 éxitos, 0 fallos**, exit code 0 |
| `BuildCookRun` Win64 Development | `BUILD SUCCESSFUL`, 42 s |
| Smoke sobre el ejecutable | `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579` |

`FirstPersonRigIsWired` cubre ahora el alternado en ambos sentidos: cámara activa,
`bOwnerNoSee` del cuerpo y visibilidad del rig de manos.

Dos fallos intermedios, ambos reales y corregidos:

1. La partida arrancaba en tercera persona. `SetActive(false)` en el constructor no alcanza
   porque los componentes se auto-activan al registrarse; hace falta `bAutoActivate = false`.
2. La aserción del estado inicial se apoyaba en `IsActive`, que en el mundo transitorio de la
   prueba no es determinista. Se cambió por la bandera del personaje; a partir del primer
   toggle sí se comprueba `IsActive`, porque ahí `ApplyCameraView` la fija explícitamente.

### Cuerpo invisible en tercera persona — corregido

| Prueba | Resultado |
|---|---|
| `AstraeonEditor Win64 Development` | Succeeded, 24,43 s |
| `Automation RunTests Astraeon` | **55 éxitos, 0 fallos** |
| `BuildCookRun` Win64 Development | `BUILD SUCCESSFUL` |
| Smoke sobre el ejecutable | `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579` |

Causa: el rig de primera persona empujaba su clip de locomoción al cuerpo de sombra, y desde
el cambio de malla ese clip vive en otro esqueleto (`SKEL_Humanoid_A`, 57 huesos, contra
`SK_Astraeon_Player_Skeleton`, 75). Evidencia previa al fix en
`ContentPipeline/reports/body_render_check.json`: `skeletons_match: false`, con materiales
correctos y opacos. La prueba ahora exige que el cuerpo reciba una animación propia y que no
pertenezca al esqueleto de las manos.

## 2026-09-08 — Protagonista visible: escala de animación y materiales

Cierre del síntoma "el personaje no se ve", abierto en `INVESTIGACION_PERSONAJE_INVISIBLE.md`.

| Prueba | Resultado |
|---|---|
| `RunCharacterChecks.ps1 -Check Audit` | mide la pose evaluada; cabeza a **1,64 cm** de los pies en los 59 clips |
| `RunCharacterChecks.ps1 -Check Repair` | `passed: true`; 59 clips normalizados y materiales con uso de malla esquelética |
| `AstraeonEditor Win64 Development` | Succeeded |
| `Automation RunTests Astraeon` | **55 éxitos, 0 fallos** |
| Smoke de cámara en editor | `AstraeonCharacterViewSmoke: Passed=true` en ambos sentidos, `HeadHeightCm=163.89 / 163.90` |
| Captura en partida, editor | `Docs/evidencia/QA_Personaje_TerceraPersona_Integrado.png`, `QA_Personaje_PrimeraPersona_Integrado.png` |
| `BuildCookRun` Win64 Development | `BUILD SUCCESSFUL`, 62,97 s |
| Smoke de cámara sobre el **ejecutable** | `Passed=true` en ambos sentidos, `HeadHeightCm=163.89 / 163.90`; capturas en `Builds/WindowsProtagonista/Astraeon/Saved/Screenshots/Windows/` |
| Recorrido crítico sobre el **ejecutable** | `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579` |

Causa: la importación FBX de animaciones sueltas perdía la conversión metros→centímetros del
Armature. El esqueleto lleva `root` a escala 100 en la pose de referencia y las claves de
animación quedaban a 1, de modo que evaluar cualquier clip encogía al personaje a 1/100.
Segunda causa concurrente: materiales generados por script sin `MATUSAGE_SKELETAL_MESH`.

El smoke de cámara ya no simula el toggle por código: inyecta la **tecla V real** por
`PlayerController::InputKey` en ambos sentidos y comprueba, un cuarto de segundo después,
cámara activa, `bOwnerNoSee` del cuerpo, visibilidad del rig de manos y altura de cabeza
entre 140 y 195 cm. La misma prueba corre sobre el editor y sobre el ejecutable.

## 2026-09-08 — Bloque A: tránsito garantizado y auditoría de arte

| Prueba | Resultado |
|---|---|
| `AstraeonEditor Win64 Development` | Succeeded |
| `Automation RunTests Astraeon` | **57 éxitos, 0 fallos** (dos nuevas: `Terrain.Connectivity` y `Terrain.TraversalDetectsWalls`) |
| `RunCharacterChecks.ps1 -Check ArtUsage` | 36 mallas auditadas; 0 slots en nulo y 0 materiales por defecto en lo que el juego usa |
| `RunCharacterChecks.ps1 -Check ArtMaterials` | 10 slots reenganchados, **verificados releyendo el asset ya guardado**, 0 sin resolver |
| Smoke de cámara en editor | `Passed=true` en ambos sentidos, `HeadHeightCm=163.90` |
| Recorrido crítico en editor | `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true` |
| `BuildCookRun` Win64 Development | `BUILD SUCCESSFUL` |
| Smoke de cámara y recorrido crítico sobre el **ejecutable** | Ambos en verde |

### Qué cubre `Astraeon.WorldGen.Terrain.Connectivity`

Siete seeds (100, 200, 300, 400, 500, 13579 y 1001). Por cada una: que la seed **publicada**
deje alcanzables a pie los cinco recursos, la señal, la anomalía y los dos nidos; que no
recurra a la variante segura; y que resolver dos veces dé el mismo relieve, porque si no,
cargar una partida devolvería otra región. Además comprueba la variante segura por separado
y el caso de Ítaca aterrizada lejos del origen.

`Astraeon.WorldGen.Terrain.TraversalDetectsWalls` existe porque una prueba de conectividad
que aprueba siempre no prueba nada: mete un objetivo fuera de la región y exige que la
validación falle y lo nombre.

### Hallazgo: la seed por defecto publicaba una región con un objetivo tapado

Con `1001`, la seed pedida deja un objetivo inalcanzable. Antes se materializaba igual.
Ahora el log de una partida normal registra el descarte y publica una sub-seed válida:

```
AstraeonTerrainSeed: seed pedida 1001 descartada tras 3 intentos; se materializa -1297777899.
Inalcanzable con la pedida: minor_geologic_anomaly
```

### Hallazgo: la reparación de materiales que decía haber funcionado

El primer intento de reenganche reportó "10 reparados" y **no persistió ninguno**: mutar los
structs que devuelve `get_editor_property('materials')` opera sobre copias. Es la misma
causa por la que los scripts de preparación creían haber asignado los materiales. Corregido
en la reparación y en los scripts de origen, y ahora ambos **releen el asset después de
guardar** antes de declarar éxito.

## 2026-09-09 — El cuerpo usa su set de animación

| Prueba | Resultado |
|---|---|
| `AstraeonEditor Win64 Development` | Succeeded |
| `Automation RunTests Astraeon` | **58 éxitos, 0 fallos** (nueva: `Art.Character.BodyAnimationSelection`) |
| Smoke de cámara en editor | `Passed=true` en ambos sentidos, `HeadHeightCm=163.90` |
| Recorrido crítico en editor | En verde |
| `BuildCookRun` Win64 Development | `BUILD SUCCESSFUL` |
| Smoke de cámara y recorrido crítico sobre el **ejecutable** | Ambos en verde |

`Astraeon.Art.Character.BodyAnimationSelection` prueba la elección de clip como función
pura —sin mundo, sin actor y sin assets— y después exige que **cada id que el selector puede
devolver exista como asset** y pertenezca al esqueleto del protagonista: un id mal escrito
dejaría al cuerpo congelado sin avisar de nada.

### Una aserción reemplazada, no debilitada

`Astraeon.Art.Character.FirstPersonRigIsWired` exigía *"Body runs while hands scan"*: que un
gesto de manos **no** tocara el cuerpo. Esa regla existía porque el cuerpo no tenía clips de
acción. Lo que protegía era que el cuerpo no se **congelara**, y eso es lo que se comprueba
ahora: el cuerpo reproduce su propio `Scan` y **vuelve a `Run_F` al terminar el clip**.
La prueba cubre además que correr de lado use `Run_L` y no `Run_F`, que es el defecto que
motivó el trabajo. AGENTS.md §7 pide exactamente esto: explicar el cambio y sustituir por una
verificación equivalente.
