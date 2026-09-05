# Informe de pruebas — ASTRAEON

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
