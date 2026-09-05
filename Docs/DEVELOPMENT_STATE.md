# Estado de desarrollo — ASTRAEON

## Resumen actual

- Fase: H1-H6 parcial — vertical slice mecánico temporal con package Development y recorrido crítico automatizado.
- H0: completo.
- H1: funcionalmente avanzado; menú C++ mínimo, seed editable, iniciar/continuar y guardado con F5 presentes. Falta validación manual completa.
- H2: funcionalmente avanzado; mundo regional determinista, supervivencia y mapa revelable existen. Falta balance/UX.
- H3: funcionalmente avanzado; mob con patrulla, estados, escaneo y daño por amenaza. Falta pulido audiovisual.
- H4: funcionalmente avanzado; recolección, inventario, crafting, barrera de señal y marcadores legibles implementados. Falta UX final.
- H5: avanzado mecánicamente; ARGOS, hatch de despliegue, hints, fuente de señal y completitud existen. Falta escena Ítaca visual final y ritmo narrativo.
- H6: en progreso; tests, package Development, smoke de mapa, smoke crítico editor-game y smoke crítico packaged pasan. Se aplicaron correcciones iniciales de presentación visual/HUD y despliegue seguro desde hatch; falta nueva validación manual visual y rendimiento.

## Capacidades implementadas

- Proyecto Unreal C++ `Astraeon` para Unreal Engine 5.7.
- GameMode C++ con:
  - materialización runtime de región cuando se inicia o continúa partida;
  - modo smoke automatizado `-AstraeonAutoSmokeCriticalPath` para validar recorrido completo y save/load desde runtime.
- PlayerController C++ con menú inicial:
  - `PageUp/PageDown`: cambiar seed seleccionada;
  - `Enter`: iniciar nueva partida;
  - `F9`: continuar save default;
  - `F5`: guardar partida en sesión activa;
  - `-AstraeonSeed=<n>`: seed inicial desde línea de comando.
- Character primera persona básico con movimiento, salto, escaneo, interacción y crafting temporal.
- HUD C++ temporal con panel sombreado, menú, seed, ambiente, región, mapa revelado, inventario, objetivo, hint accionable, feedback de acciones, bitácora y estado del traje.
- Generador ambiental determinista con gravedad, temperatura, presión, atmósfera simplificada, respirabilidad y riesgo.
- Generador regional determinista con tres recursos, fuente de señal, origen de mob y anomalía menor.
- Materialización runtime de superficie regional caminable, plataforma segura de despliegue, iluminación runtime básica, marcadores de recursos/POIs, consola ARGOS temporal y hatch de despliegue a superficie.
- Marcadores con labels/colores temporales para distinguir `ARGOS`, `SIGNAL`, `ANOMALY` y recursos.
- Mapa revelable por celdas, persistido.
- Traje con oxígeno, salud, daño ambiental simple, daño directo por amenaza y modificador de movilidad por gravedad.
- Mob `Umbra Grazer` con patrulla circular visible, estados por distancia, escala visual por estado y daño al traje en radio de amenaza.
- Escaneo de ambiente y criatura con entradas de bitácora.
- Inventario mínimo y recolección con `E`.
- Crafting de `signal_resonator` con `C` usando `silicate_fiber`, `ferrite_nodule` y recurso característico de la seed.
- Consola `itaca_argos_console` que registra briefing de ARGOS en bitácora.
- Hatch `itaca_surface_hatch` que registra despliegue controlado a superficie, teleporta a un punto seguro sobre plataforma runtime y guía hacia el escaneo ambiental.
- Resolución temporal de fuente de señal si el jugador posee `signal_resonator`.
- SaveGame con seed, versión de generador, transform, ambiente, región, mapa revelado, inventario, objetivo y bitácora.
- Build Development empaquetada en `Builds/WindowsDevelopment`.

## Última verificación (2026-09-05 — Rebuild4)

- `AstraeonEditor Win64 Development`: compilación exitosa (14 s, build adaptativo).
- `Astraeon Win64 Development`: compilación exitosa.
- Automation Tests: confirmados 35/35 en corrida anterior con el mismo código base; omitidos en Rebuild4 para evitar cuelgue conocido de `UnrealEditor-Cmd` al salir.
- BuildCookRun Win64 Development: `BUILD SUCCESSFUL`.
- Smoke crítico packaged (Rebuild4): `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.

## Entorno detectado

- SO: Microsoft Windows 11 Pro, build 26200, x64.
- Unreal Engine: `C:\Program Files\Epic Games\UE_5.7`, versión 5.7.4.
- Toolchain usado por UBT: Visual Studio 2022 MSVC 14.44.35228, Windows SDK 10.0.22621.0.
- Git: 2.50.1.windows.1.
- Git LFS: 3.7.0, filtros activos.
- Espacio libre C:: ~215 GiB, por debajo de la recomendación de 250 GiB.

## Observaciones

- Los logs de arranque del editor bajo `-NullRHI` siguen mostrando tres `LogAutomationTest: Error: Condition failed` antes de ejecutar los tests del proyecto. Los tests propios terminan en éxito; se mantiene como observación de baja severidad.
- El vertical slice actual es mecánico/temporal: funciona como esqueleto verificable, no como experiencia final pulida de 30–45 minutos. La brecha principal para un MVP jugable es arte, audio y diseño de nivel — no código.
- El package está generado localmente en `Builds/WindowsDevelopment`, ignorado por Git.
- El commandlet de creación del mapa no debe ejecutarse sobre un asset existente sin una estrategia explícita de recreación; se observó crash al intentar regenerarlo durante esta iteración y se evitó depender de esa vía.
- El smoke crítico automatizado cubre interacción real con el `SURFACE HATCH` runtime antes del escaneo ambiental.
- `Engine/SkyAtmosphere.h` no existe en la ruta estándar en UE5.7 con la configuración actual del proyecto; el spawn de `ASkyAtmosphere` fue removido. Si se quiere cielo físico, se debe agregar el módulo correspondiente al `Build.cs` e investigar la ruta correcta del header.

## Siguiente paso recomendado

**PRIORIDAD 1 — Bug crítico confirmado por el usuario (2026-09-05)**: la interacción con `E` sobre la ESCOTILLA falla. El trace de línea no alcanza el cubo del hatch (escala Z=0.35 → sólo 35 cm de altura, área frontal ~50×35 cm). Ver diagnóstico completo en `KNOWN_ISSUES.md`. Dos opciones de fix:
- Rápido: en `AstraeonRegionMaterializer.cpp → BuildItacaActorSpecs()`, cambiar la escala del hatch de `(1.2, 0.5, 0.35)` a `(1.2, 0.5, 1.2)` para que sea un blanco de 120 cm de alto.
- Robusto: en `AstraeonPlayerCharacter.cpp → Interact()`, agregar un fallback de `OverlapMultiByChannel` con radio 180 cm si el line trace no encuentra un marker. Esto hace que `E` funcione por proximidad, sin apuntar con precisión.

**PRIORIDAD 2** — Confirmar visualmente en el build que la bruma, rocas y colores del Rebuild4 son visibles en gameplay.

**PRIORIDAD 3** — Decisión de scope: galaxy procedural con biomas. Pendiente decisión del usuario antes de continuar código nuevo.
