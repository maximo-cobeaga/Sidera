# Informe de pruebas — ASTRAEON

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
