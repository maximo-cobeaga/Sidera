# Handoff — ESCOTILLA no responde en smoke manual

## Estado

**Bloqueante manual confirmado.** El usuario reportó dos veces que la ESCOTILLA no funciona en juego real. Después de Rebuild6, estando frente a la ESCOTILLA y presionando `E`, no ocurre ninguna acción visible.

Esto debe tratarse como bug del juego/UX, no como error del jugador. El MVP no puede depender de puntería exacta, posición secreta ni conocimiento interno.

## Build afectado

- Ejecutable probado por el usuario: `Builds/WindowsDevelopment/Astraeon.exe`.
- Rebuild6 fue empaquetado correctamente después de cerrar una instancia anterior de `Astraeon.exe` que bloqueaba el archive.
- Smoke automatizado packaged con `-nullrhi` pasa, pero no reproduce la interacción manual real.

## Evidencia automática previa

Rebuild6 pasó:

- `AstraeonEditor Win64 Development`: OK.
- `Astraeon Win64 Development`: OK.
- Automation Tests `Astraeon.*`: 35/35 OK.
- Critical path smoke editor-game: OK.
- BuildCookRun Win64 Development: `BUILD SUCCESSFUL`.
- Critical path smoke packaged: OK.

Log esperado observado:

```text
AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579
```

## Cambios ya aplicados en Rebuild6

Archivos principales:

- `Source/Astraeon/Private/AstraeonPlayerCharacter.cpp`
- `Source/Astraeon/Public/AstraeonPlayerCharacter.h`
- `Source/Astraeon/Private/WorldGen/AstraeonRegionMaterializer.cpp`
- `Source/Astraeon/Private/AstraeonGameModeBase.cpp`
- `Source/Astraeon/Private/Tests/AstraeonCoreAutomationTests.cpp`

Cambios:

- `AAstraeonPlayerCharacter::Interact()` conserva line trace directo.
- Si el trace no pega a `AAstraeonRegionMarker`, usa `FindBestRegionMarkerInReach`.
- Radio de fallback: `450 cm`.
- Fallback prioriza marcador cercano a la mira (`AimForgivenessRadiusCm = 160 cm`) y marcador muy cercano (`CloseInteractionRadiusCm = 220 cm`).
- La ESCOTILLA temporal aumentó altura visual/colisión: escala Z de `0.35` a `1.2`.
- El smoke de hatch coloca al personaje en `(360, 0, 120)` y mira horizontalmente por encima del cubo.

## Hipótesis actuales

Investigar en este orden:

1. **Input manual `E` no llega a `AAstraeonPlayerCharacter::Interact()` en packaged build.**
   - `Config/DefaultInput.ini` tiene `+ActionMappings=(ActionName="Interact", Key=E)`.
   - El binding actual está en `AAstraeonPlayerCharacter::SetupPlayerInputComponent`.
   - Confirmar con log temporal o feedback HUD específico al entrar a `Interact()`.

2. **La sesión no está activa o el jugador sigue en estado de menú aunque visualmente parezca estar jugando.**
   - `AAstraeonPlayerController::StartSelectedNewGame()` pone `bMenuVisible=false` y llama `ApplySessionToRuntime()`.
   - Confirmar que el HUD muestre sesión activa/objetivo y que `UAstraeonGameInstance::HasStartedGame()` sea true.

3. **El marker visible de ESCOTILLA no está en la posición/radio esperados o no es un `AAstraeonRegionMarker` interactuable.**
   - Revisar spawn en `UAstraeonRegionMaterializer`.
   - Confirmar `MarkerId == "itaca_surface_hatch"` y `MarkerKind` correcto.
   - Agregar debug temporal en pantalla/log con distancia al hatch y marker encontrado.

4. **El fallback automático pasa por coordenadas/headless pero no por cámara manual.**
   - El smoke no prueba una tecla real ni una frame interactiva con input de usuario; llama `PlayerCharacter->Interact()` directamente.
   - Hace falta un test/manual debug que distinga “input no llega” de “interacción falla”.

5. **El jugador está probando un build viejo.**
   - Ya ocurrió que Windows bloqueó el archive porque `Astraeon.exe` estaba abierto.
   - Antes de repackagear, cerrar toda instancia de ASTRAEON y confirmar con `Get-Process Astraeon`.

## Recomendación para el próximo agente

Hacer un fix diagnóstico primero, no seguir ampliando radios a ciegas.

### Paso 1 — Instrumentación temporal visible

En `AAstraeonPlayerCharacter::Interact()`:

- Al comienzo: `SetLastFeedbackMessage("DEBUG: E recibido")` o log claro.
- Si falla marker: mostrar distancia al hatch y cantidad de markers encontrados.
- Si encuentra marker: mostrar `MarkerId`.

Esto permite saber en una sola prueba manual si el problema es input, marker o condición de despliegue.

### Paso 2 — Si input no llega

Evaluar rutear `Interact` desde `AAstraeonPlayerController` y evitar doble ejecución:

- Bindear `Interact` en `AAstraeonPlayerController::SetupInputComponent()`.
- Llamar a `AAstraeonPlayerCharacter::Interact()` sólo si `bMenuVisible == false`.
- Remover o proteger el binding duplicado en `AAstraeonPlayerCharacter`.
- Compilar y probar manualmente.

Un intento de esta idea fue iniciado pero revertido en esta sesión porque el usuario pidió parar y dejar handoff.

### Paso 3 — Si input llega pero marker no

Agregar fallback explícito de ESCOTILLA por zona:

- Usar una constante central para la ubicación de la hatch de Ítaca o exponerla desde `UAstraeonRegionMaterializer`.
- Si `HasStartedGame()` es true y el jugador está cerca de la zona de la hatch, llamar a `DeployToSurface()` aunque no haya marker hit.
- Idealmente requerir que la hatch esté aproximadamente frente al jugador, pero con tolerancia amplia.

### Paso 4 — Prueba de aceptación manual

La ESCOTILLA se considera corregida sólo si el usuario puede:

1. Abrir `Builds/WindowsDevelopment/Astraeon.exe` sin `-nullrhi`.
2. `Enter` para nueva partida.
3. Pararse frente o cerca de `ESCOTILLA`.
4. Presionar `E` una sola vez.
5. Ver cambio de `Estado:` y despliegue a superficie.

## Cuidado con el alcance

No convertir esto en sistema final de interacción, UI final o escena final de Ítaca. Resolver primero el bloqueo jugable mínimo: `E` sobre ESCOTILLA debe funcionar en build manual.

## Nota sobre review/commit

El árbol contiene cambios Rebuild6 aún sin commit. Además existen archivos sin trackear aparentemente ajenos:

- `Docs/GUIA_ARTE_PLANETAS_BLENDER.md`
- `graphics/astra_test.blend`
- `graphics/test_astra.py`

No incluirlos en un commit de fix de ESCOTILLA salvo decisión explícita del propietario.
