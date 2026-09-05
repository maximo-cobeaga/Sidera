# Estado de desarrollo — ASTRAEON

## Resumen actual

- Fase: H1-H6 parcial — vertical slice mecánico temporal con package Development y recorrido crítico automatizado.
- H0: completo.
- H1: funcionalmente avanzado; menú C++ mínimo, seed editable, iniciar/continuar y guardado con F5 presentes. Falta validación manual completa.
- H2: funcionalmente avanzado; mundo regional determinista, supervivencia y mapa revelable existen. Falta balance/UX.
- H3: funcionalmente avanzado; mob con patrulla, estados, escaneo y daño por amenaza. Falta pulido audiovisual.
- H4: funcionalmente avanzado; recolección, inventario, crafting, barrera de señal y marcadores legibles implementados. Falta UX final.
- H5: avanzado mecánicamente; ARGOS, hints, fuente de señal y completitud existen. Falta escena Ítaca/transición presentable y ritmo narrativo.
- H6: en progreso; tests, package Development, smoke de mapa, smoke crítico editor-game y smoke crítico packaged pasan. Falta smoke manual y rendimiento.

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
- HUD C++ temporal con menú, seed, ambiente, región, mapa revelado, inventario, objetivo, hint accionable, bitácora y estado del traje.
- Generador ambiental determinista con gravedad, temperatura, presión, atmósfera simplificada, respirabilidad y riesgo.
- Generador regional determinista con tres recursos, fuente de señal, origen de mob y anomalía menor.
- Materialización runtime de marcadores para recursos, POIs y consola ARGOS temporal.
- Marcadores con labels/colores temporales para distinguir `ARGOS`, `SIGNAL`, `ANOMALY` y recursos.
- Mapa revelable por celdas, persistido.
- Traje con oxígeno, salud, daño ambiental simple, daño directo por amenaza y modificador de movilidad por gravedad.
- Mob `Umbra Grazer` con patrulla circular visible, estados por distancia, escala visual por estado y daño al traje en radio de amenaza.
- Escaneo de ambiente y criatura con entradas de bitácora.
- Inventario mínimo y recolección con `E`.
- Crafting de `signal_resonator` con `C` usando `silicate_fiber`, `ferrite_nodule` y recurso característico de la seed.
- Consola `itaca_argos_console` que registra briefing de ARGOS en bitácora.
- Resolución temporal de fuente de señal si el jugador posee `signal_resonator`.
- SaveGame con seed, versión de generador, transform, ambiente, región, mapa revelado, inventario, objetivo y bitácora.
- Build Development empaquetada en `Builds/WindowsDevelopment`.

## Última verificación

- `AstraeonEditor Win64 Development`: compilación exitosa.
- `Astraeon Win64 Development`: compilación exitosa.
- Automation Tests `Astraeon.*`: 33 encontrados, 33 exitosos, exit code 0.
- Smoke crítico editor-game: `AstraeonCriticalPathSmoke: Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- BuildCookRun Win64 Development: `BUILD SUCCESSFUL`.
- Smoke crítico packaged: `AstraeonCriticalPathSmoke: Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- Smoke de mapa por Python commandlet previo: `L_AstraeonBootstrap` carga, MapCheck reporta 0 errores/0 advertencias, actores mínimos presentes, exit code 0.

## Entorno detectado

- SO: Microsoft Windows 11 Pro, build 26200, x64.
- Unreal Engine: `C:\Program Files\Epic Games\UE_5.7`, versión 5.7.4.
- Toolchain usado por UBT: Visual Studio 2022 MSVC 14.44.35228, Windows SDK 10.0.22621.0.
- Git: 2.50.1.windows.1.
- Git LFS: 3.7.0, filtros activos.
- Espacio libre C:: ~215 GiB, por debajo de la recomendación de 250 GiB.

## Observaciones

- Los logs de arranque del editor bajo `-NullRHI` siguen mostrando tres `LogAutomationTest: Error: Condition failed` antes de ejecutar los tests del proyecto. Los tests propios terminan en éxito; se mantiene como observación de baja severidad.
- El vertical slice actual es mecánico/temporal: funciona como esqueleto verificable, no como experiencia final pulida de 30–45 minutos.
- El package está generado localmente en `Builds/WindowsDevelopment`, ignorado por Git.
- Falta una verificación visual/manual en viewport o ejecutable para confirmar escala, HUD, labels, interacción, percepción de criatura y claridad de objetivos.

## Siguiente paso recomendado

Ejecutar smoke manual del package. Si es aceptable, avanzar con informe final provisional; si no, corregir escala/legibilidad/interacciones detectadas.
