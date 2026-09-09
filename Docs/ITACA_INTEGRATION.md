# Ítaca Q1 — integración y diagnóstico

## Resultado y composición

`AAstraeonItacaInterior` ensambla el mismo kit en `/Game/Maps/TL_Art_MVP` y en el recorrido de `/Game/Maps/L_AstraeonBootstrap`. El mapa bootstrap binario no se reemplaza: su GameMode crea el interior si todavía no existe. Iniciar otra partida o rematerializar región no duplica la habitación.

Volumen métrico: X de -180 a 620 cm, Y de -300 a 300 cm, suelo Z=0, techo Z=280. Cápsula de radio 42 cm/semialtura 96 cm; cámara +64 cm desde su centro. Doce paneles de suelo y doce de techo, paredes modulares, marco, iluminación local y nota personal. Altura/anchura libre de puerta: 220/130 cm. Las superficies visuales no deciden la colisión: cajas de suelo/pared/laterales/dintel dejan el hueco libre.

ARGOS conserva ID `itaca_argos_console`, XY (250,-140), con malla original de 80×60×120 cm y colisión de consola. ESCOTILLA conserva ID `itaca_surface_hatch`, XY (620,0), con un blanco de interacción invisible que responde a Visibility y no bloquea Pawn. El marco visible pertenece al interior; no se teletransporta ni desaparece al usarlo. El destino continúa en XY (0,1200), sobre la plataforma segura. Se excluyen rocas del volumen de Ítaca con margen por tamaño del prop.

Es un blockout jugable, no arte final. Continúan pendientes el ritmo narrativo, interfaz final, audio y validación humana. La puerta abierta permite ensayar el gálibo; no añade una simulación de esclusa ni vuelo.

## Causa reproducida de la escotilla

El nuevo smoke envía Enter/E al `APlayerController` y deja procesar frames. En `Saved/Logs/ItacaInputBaseline.log`:

1. Enter inicia sesión, ARGOS recibe E y registra briefing.
2. E encuentra `itaca_surface_hatch`.
3. Aparece `Calling SetStaticMesh ... but Mobility is Static` al materializar el suelo/plataforma/rocas después del inicio.
4. Ambos traces de piso de destino fallan y no hay teleport. El nuevo smoke declara `Passed=false`.

El smoke antiguo creaba región durante `BeginPlay`, antes del flujo real de menú. Por eso no reproducía el rechazo de `SetStaticMesh` después del arranque. La lectura del código local de `UStaticMeshComponent::SetStaticMesh` y el log redujeron el problema a esta diferencia temporal; no fue necesario cambiar el binding de E ni ampliar radios.

Corrección: configurar `Movable` antes de asignar meshes a los `AStaticMeshActor` creados en runtime. La repetición `ItacaInputFixed.log` llega al destino, permanece sobre piso y devuelve `Passed=true`, sin warnings de movilidad. El registro de despliegue se hace después de validar suelo y realizar la transición, para no avanzar progreso cuando falta piso.

Esto demuestra un fallo real y su corrección por la ruta de entrada del controlador. No equivale a probar un teclado físico ni a confirmación humana de todos los fallos reportados anteriormente.

## Reproducir

1. Si faltan los FBX, generar el kit según `BLENDER_PIPELINE.md`.
2. Ejecutar `Scripts/Editor/PrepareItacaBlockoutAssets.py` con `UnrealEditor-Cmd -run=pythonscript` para persistir assets originales en `/Game/Astraeon/Art/Blockouts/Itaca`. Reutiliza el importador validado y verifica procedencia por hash antes de aceptar assets existentes; no reimporta silenciosamente modificaciones.
3. Compilar `AstraeonEditor Win64 Development`.
4. Ejecutar `Scripts/Editor/CreateItacaArtTestMap.py`. Crea o completa su fixture etiquetado, preservando otros actores. Evita `spawn_actor_from_object`, que produjo un access violation en commandlet; usa `StaticMeshActor` y asignación explícita de malla.
5. Ejecutar desde la raíz:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/RunItacaChecks.ps1 -Check Automation
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/RunItacaChecks.ps1 -Check InputArt
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/RunItacaChecks.ps1 -Check InputBootstrap
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/RunItacaChecks.ps1 -Check CriticalPath
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/RunItacaChecks.ps1 -Check Package
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/RunItacaChecks.ps1 -Check PackagedInput
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/RunItacaChecks.ps1 -Check PackagedCritical
powershell -NoProfile -ExecutionPolicy Bypass -File Scripts/RunItacaChecks.ps1 -Check PackagedVisual
```

`InputArt` y `InputBootstrap` comprueban una sola habitación, sweep de cápsula por hueco/lateral/dintel y altura relativa de cámara; luego usan E para ARGOS, W para caminar por el marco y E para desplegar. Tras la transición comprueban posición y estado Walking. No llaman a `Interact()` directamente. `CriticalPath` conserva la verificación independiente de escaneo, crafting, señal y serialización.

Los scripts sólo cierran sus procesos de prueba, reportan exit code y marcador de éxito; no aceptan sólo que el ejecutable haya terminado. Un timeout falla. Las capturas `Visual`/`PackagedVisual` usan el renderer real fuera de pantalla, ocultando HUD sólo para revisar arte; no miden rendimiento interactivo. Los JSON de `ContentPipeline/reports/itaca_*.json` indican las corridas realizadas.

El package nuevo va a `Builds/WindowsItaca`, conservando `WindowsDevelopment`. El launcher para prueba humana es `Builds/WindowsItaca/Astraeon.exe`. Controles: Enter, WASD/ratón, E en ARGOS y ESCOTILLA, click izquierdo, C, F5/F9. El diagnóstico previo del HUD se conserva.
