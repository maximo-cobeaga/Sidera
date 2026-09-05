# Backlog — ASTRAEON MVP La primera señal

## Estado de hitos

| Hito | Estado | Criterio de salida |
| --- | --- | --- |
| H0 — Bootstrap reproducible | Completo | Proyecto C++ creado, Git/LFS configurados, build del editor/juego, mapa bootstrap y tests base pasan. |
| H1 — Caminata vertical | En progreso avanzado | Primera persona, menú, seed, medición, bitácora y guardado mínimo. Falta validación manual completa. |
| H2 — Mundo y supervivencia | En progreso avanzado | Región procedural, ambiente completo, consecuencias del traje y mapa revelable. Falta balance/UX. |
| H3 — Vida y conocimiento | En progreso avanzado | Mob evitable, escaneo progresivo y bitácora integrada. Falta pulido audiovisual. |
| H4 — Recursos y solución | En progreso avanzado | Tres recursos, inventario, receta, barrera superable y marcadores legibles. Falta UX final. |
| H5 — La primera señal | En progreso avanzado | ARGOS, objetivo final y recorrido completo existen. Falta escena Ítaca/transición presentable y ritmo narrativo. |
| H6 — Candidato MVP | En progreso | Tests, critical-path smoke y package Development existen. Faltan smoke manual, rendimiento e informe final. |

## Completado

- **H0.1 — Validar bootstrap C++**: `AstraeonEditor`/`Astraeon` compilan; tests base pasan.
- **H0.2 — Crear mapa bootstrap reproducible**: `L_AstraeonBootstrap.umap` generado y smoke headless pasa.
- **H1.1 — Nueva partida con seed**: `UAstraeonGameInstance::StartNewGame` genera ambiente determinista y entrada inicial de bitácora.
- **H1.2 — Bitácora y guardado mínimo integrados**: save/load round-trip de seed, ambiente, región, mapa, inventario, objetivo y bitácora verificado.
- **H1.3 — Arranque jugable temporal**: GameMode, PlayerController y HUD C++ integrados.
- **H1.4 — Interacción mínima de escaneo**: acción `Scan` confirma medición ambiental y mejora la bitácora.
- **H1.5 — Menú inicial mínimo**: menú HUD C++ con seed editable (`PageUp/PageDown`), nueva partida (`Enter`), continuar (`F9`) y seed por línea de comando `-AstraeonSeed=<n>`.
- **H2.1 — Modelo de región procedural acotada**: layout determinista con tres recursos, fuente de señal, spawn de mob y anomalía menor; persistido en SaveGame.
- **H2.2 — Materializar región en mapa**: GameMode spawnea marcadores runtime para recursos y POIs desde el layout.
- **H2.3 — Consecuencias ambientales iniciales**: componente de traje consume oxígeno, aplica daño ambiental simple y modifica movilidad por gravedad.
- **H2.4 — Mapa revelable mínimo**: celdas reveladas alrededor del inicio y por escaneo, persistidas en SaveGame.
- **H3.1 — Mob mínimo**: criatura runtime con estados de patrulla, alerta, amenaza y desinterés según distancia.
- **H3.2 — Escaneo de mob**: escaneo bajo mira registra criatura en bitácora.
- **H3.3 parcial — Mob jugable más claro**: patrulla circular visible, escala por estado y daño al traje si el jugador entra en radio de amenaza.
- **H4.1 — Inventario/recolección mínima**: `E` recolecta marcadores de recurso y los acumula en inventario.
- **H4.2 — Crafting mínimo**: `C` fabrica `signal_resonator` consumiendo fibra, ferrita y recurso característico de la seed; registra receta.
- **H4.3 parcial — Barrera/markers explícitos**: `signal_source` explica que requiere `signal_resonator`; marcadores tienen label/color para ARGOS, señal, anomalía y recursos.
- **H5.1 parcial — Fuente de señal**: interactuar con `signal_source` completa el objetivo si existe `signal_resonator` y registra hallazgo final.
- **H5.2 parcial — ARGOS mínimo**: consola runtime `itaca_argos_console` registra briefing de misión en bitácora.
- **H5.3 parcial — Guía de objetivos**: HUD muestra hint accionable por objetivo.
- **H6.1 — Package Development**: BuildCookRun Win64 Development exitoso y smoke headless del ejecutable empaquetado.
- **H6.2 parcial — Recorrido crítico automatizado**: test `Astraeon.Functional.CriticalPath.FullFlow` y flag runtime `-AstraeonAutoSmokeCriticalPath` verifican briefing → scan → recursos → crafting → señal → save/load.

## Próximas tareas desbloqueadas

1. **Smoke manual obligatorio**
   - Ejecutar `Builds/WindowsDevelopment/Astraeon.exe` sin `-nullrhi`.
   - Validar menú, escala, labels, interacción, mob, guardado y completitud.

2. **H5.2 restante — Ítaca/narrativa mínima**
   - Estancia inicial o zona interior simple.
   - Consola de misión/ARGOS con mejor ubicación y lectura.
   - Transición controlada a región.

3. **H6 — Candidato MVP**
   - Medición básica de rendimiento.
   - Informe final y limitaciones conocidas.
   - Ajustes derivados del smoke manual.

4. **Pulido futuro no bloqueante**
   - Sonido/partículas/UI de alerta.
   - Arte final y mejor composición visual.
