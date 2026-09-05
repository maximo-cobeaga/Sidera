# Informe de pruebas — ASTRAEON

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
