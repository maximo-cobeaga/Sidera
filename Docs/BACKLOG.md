# Backlog — ASTRAEON MVP La primera señal

## Estado de hitos

| Hito | Estado | Criterio de salida |
| --- | --- | --- |
| H0 — Bootstrap reproducible | Completo | Proyecto C++ creado, Git/LFS configurados, build del editor/juego, mapa bootstrap y tests base pasan. |
| H1 — Caminata vertical | En progreso avanzado | Primera persona, menú, seed, medición, bitácora y guardado mínimo. Falta validación manual completa. |
| H2 — Mundo y supervivencia | En progreso avanzado | Región procedural, ambiente completo, consecuencias del traje y mapa revelable. Falta balance/UX. |
| H3 — Vida y conocimiento | En progreso avanzado | Mob evitable, escaneo progresivo y bitácora integrada. Falta pulido audiovisual. |
| H4 — Recursos y solución | En progreso avanzado | Tres recursos, inventario, receta, barrera superable y marcadores legibles. Falta UX final. |
| H5 — La primera señal | En progreso avanzado | ARGOS, hatch de despliegue, objetivo final y recorrido completo existen. Falta escena Ítaca visual final y ritmo narrativo. |
| H6 — Candidato MVP | En progreso | Tests, critical-path smoke y package Development existen. Faltan smoke manual visual, rendimiento e informe final. |

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
- **H5.2 parcial — Transición controlada temporal**: hatch runtime `itaca_surface_hatch` registra despliegue controlado, guía el flujo desde Ítaca y teleporta al jugador fuera del stub inicial.
- **H5.3 parcial — Guía de objetivos**: HUD muestra hint accionable por objetivo, incluyendo briefing ARGOS y hatch antes del escaneo.
- **H6.1 — Package Development**: BuildCookRun Win64 Development exitoso y smoke headless del ejecutable empaquetado.
- **H6.2 parcial — Recorrido crítico automatizado**: test `Astraeon.Functional.CriticalPath.FullFlow` y flag runtime `-AstraeonAutoSmokeCriticalPath` verifican briefing → interacción real con hatch/despliegue → scan → recursos → crafting → señal → save/load.
- **H6.3 parcial — Corrección inicial de presentación visual**: iluminación runtime básica, mensajes de feedback en HUD propio y panel/espaciado de HUD para evitar pantalla negra y textos solapados.
- **H6.4 — Despliegue seguro desde hatch**: el hatch usa un punto de despliegue centralizado, detiene movimiento al teleportar, existe una plataforma runtime dedicada bajo el destino, y el fix anti-caída quedó **confirmado por el usuario en juego real** (ya no se cae del mapa).
- **H6.5 — Mira y feedback de interacción legible**: se agregó una mira central al HUD y color distintivo para la línea `Feedback:`, como corrección para los reportes de que `E` sobre el hatch/recursos "no parece hacer nada" (diagnóstico: faltaba referencia de apuntado, no una falla de lógica). Pendiente de recompilar/probar.

## Próximas tareas desbloqueadas

1. **Smoke manual obligatorio (mira/feedback)**
   - Recompilar `AstraeonEditor`/`Astraeon` (cambio pendiente en `AstraeonHUD.cpp`, ver `Docs/DECISIONS.md` 2026-09-05 "Confirmación manual del fix anti-caída y mira/feedback de interacción") y volver a empaquetar `Builds/WindowsDevelopment` antes de probar.
   - Ejecutar `Builds/WindowsDevelopment/Astraeon.exe` sin `-nullrhi`.
   - Usando la mira nueva, apuntar deliberadamente al `SURFACE HATCH` y presionar `E`: confirmar que `Logbook entries` sube y que `Feedback:` (verde) cambia a "Surface deployment recorded...".
   - Apuntar a un recurso ya escaneado y presionar `E`: confirmar que `Inventory stacks` sube y `Feedback:` muestra "Collected: ...".
   - La caída del mapa por `SURFACE HATCH` ya está confirmada resuelta por el usuario; no requiere reverificación, sólo mantener la red de rescate como red general.
   - Nota aparte, no bloqueante: la vista se ve casi negra a pocos pasos del punto de despliegue por falta de atmósfera/niebla (ver `KNOWN_ISSUES.md`); no evaluar como regresión del fix de mira.

2. **H5.2 restante — Ítaca/narrativa mínima**
   - Reemplazar los marcadores temporales por una estancia visualmente más presentable.
   - Mejorar lectura/ritmo del briefing ARGOS.
   - Confirmar en smoke manual que la transición hatch → región es clara.

3. **H6 — Candidato MVP**
   - Medición básica de rendimiento.
   - Informe final y limitaciones conocidas.
   - Ajustes derivados del smoke manual.

4. **Pulido futuro no bloqueante**
   - Sonido/partículas/UI de alerta.
   - Arte final y mejor composición visual.
