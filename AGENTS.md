# Contrato operativo de agentes — ASTRAEON

Este archivo contiene instrucciones permanentes para cualquier agente que trabaje en este repositorio. Tiene prioridad sobre instrucciones improvisadas, salvo orden explícita del propietario.

## 1. Misión

Construir y verificar el MVP **La primera señal** de **ASTRAEON: Legado del Vacío** en Unreal Engine 5.7 para Windows 11.

La experiencia es una aventura espacial científica en primera persona. El núcleo jugable es:

> explorar → medir → comprender → actuar → registrar

La ciencia debe funcionar como herramienta y ventaja, nunca como examen escolar.

## 2. Fuentes de verdad

Antes de actuar, leer en este orden:

1. `AGENTS.md`
2. `Docs/MVP_LA_PRIMERA_SENAL.md`
3. `Docs/MVP_WORLD_ARCHITECTURE.md`, si existe
4. `Docs/DEVELOPMENT_STATE.md`, si existe
5. `Docs/BACKLOG.md`, si existe
6. `Docs/GAME_DESIGN_MASTER.md`
7. ADR relevantes dentro de `Docs/ADR/`

En caso de conflicto:

1. Seguridad e integridad del repositorio.
2. Alcance y criterios de aceptación del MVP.
3. Contrato operativo de este archivo.
4. Documento maestro del juego.
5. Preferencias de implementación registradas posteriormente.

No reinterpretar una idea del juego completo como requisito inmediato del MVP.

## 3. Autonomía esperada

Trabajar sin solicitar aclaraciones menores. Frente a una ambigüedad no bloqueante:

1. Elegir la alternativa más simple, reversible, verificable y extensible.
2. Evitar añadir dependencias.
3. Registrar la decisión en `Docs/DECISIONS.md` o mediante un ADR.
4. Continuar.

Consultar al propietario únicamente cuando:

- La decisión cambie sustancialmente la experiencia del jugador.
- Dos requisitos obligatorios sean incompatibles.
- Sea necesario pagar, iniciar sesión, aceptar una licencia o suministrar credenciales.
- Falte una dependencia que no pueda instalarse dentro de la autorización existente.
- Haya riesgo material de pérdida de datos.
- Se requiera ampliar el alcance del MVP.
- Un bloqueo persista después de tres estrategias técnicamente distintas y documentadas.

## 4. Restricciones técnicas

- Motor: Unreal Engine 5.7, versión fijada durante todo el MVP.
- Plataforma objetivo: Windows 11 x64.
- Núcleo del gameplay: C++.
- Blueprints: presentación, composición visual y configuración delgada.
- Enhanced Input para controles.
- Un solo jugador y funcionamiento offline.
- Generación procedural determinista mediante seeds explícitas.
- Unidades físicas documentadas y, preferentemente, SI internamente.
- Datos de contenido separados de la lógica mediante estructuras, Data Assets o tablas cuando resulte apropiado.
- Sistemas desacoplados mediante componentes, subsistemas e interfaces.
- No introducir servicios de IA en tiempo de ejecución durante el MVP.
- No introducir plugins pagos.
- No descargar ni redistribuir assets sin licencia comprobable.
- No actualizar Unreal Engine, toolchain ni dependencias durante el MVP sin un ADR aprobado.

La lógica imprescindible no puede existir exclusivamente en un `.uasset` binario si puede representarse de forma mantenible en código o configuración textual.

## 5. Límites del MVP

Implementar únicamente lo detallado en `Docs/MVP_LA_PRIMERA_SENAL.md`.

Está prohibido incorporar durante esta fase:

- Galaxia completa o universo infinito.
- Planetas esféricos totalmente transitables.
- Viajes interestelares jugables.
- Civilizaciones procedurales completas.
- Religiones, idiomas o política sistémica.
- Colonización, ejércitos o guerras planetarias.
- Reproducción, hijos, híbridos o sucesión por muerte.
- Multijugador.
- Economía avanzada.
- Farming completo.
- Construcción libre extensa.
- Arte final o cinemáticas complejas.

Se permite crear interfaces o estructuras mínimas que faciliten expansiones futuras, pero no implementar las expansiones.

## 6. Ciclo autónomo obligatorio

En cada iteración:

1. Leer el estado, backlog, criterios de aceptación y últimos logs.
2. Elegir la tarea desbloqueada de mayor prioridad.
3. Definir el cambio mínimo que deje una capacidad completa y comprobable.
4. Crear o actualizar pruebas antes o junto con la implementación.
5. Implementar.
6. Compilar el target afectado.
7. Ejecutar las pruebas relevantes.
8. Ejecutar smoke test si el cambio afecta integración, mapa o flujo principal.
9. Analizar errores, warnings nuevos, crashes y resultados.
10. Corregir y repetir hasta pasar o declarar bloqueo real.
11. Actualizar `Docs/DEVELOPMENT_STATE.md`, `Docs/BACKLOG.md`, `Docs/TEST_REPORT.md` y deuda técnica.
12. Crear un commit atómico sólo si el estado queda estable.
13. Continuar automáticamente con la siguiente tarea.

No medir avance por cantidad de archivos ni líneas de código. Medirlo por criterios jugables aprobados.

## 7. Estrategia de pruebas

Mantener, como mínimo:

- Unit tests para fórmulas, conversiones y reglas científicas.
- Tests deterministas: misma seed → mismos resultados esenciales.
- Tests de invariantes del generador: valores dentro de rangos válidos.
- Tests de serialización y compatibilidad de guardados.
- Automation Tests para componentes principales.
- Smoke test de apertura del mapa y comienzo de partida.
- Functional test del recorrido crítico cuando sea posible.
- Build Development empaquetada antes de cerrar cada hito.

No desactivar una prueba para lograr una compilación verde. Si una prueba deja de ser válida, explicar el cambio y reemplazarla por una verificación equivalente.

## 8. Presupuesto de rendimiento del MVP

Objetivos iniciales, revisables mediante ADR:

- Resolución de referencia: 1920×1080.
- Hardware de referencia: i5-14400F, RTX 4060 8 GB, 16–32 GB RAM.
- Objetivo jugable: 60 FPS en calidad media/alta; mínimo aceptable provisional: 45 FPS sin stutter persistente.
- Tiempo de inicio del build: objetivo menor a 45 segundos en SSD.
- Ausencia de crecimiento ilimitado de memoria durante una sesión completa.
- Generación regional sin bloquear visiblemente el game thread durante períodos prolongados.

Primero se mide; después se optimiza.

## 9. Diseño procedural

Toda aleatoriedad jugable debe provenir de streams deterministas derivados de una seed raíz. No utilizar aleatoriedad global no controlada para contenido persistente.

Principios:

- Generar mediante causas, no combinaciones cosméticas independientes.
- Separar `WorldSeed`, `SystemSeed`, `BodySeed`, `RegionSeed`, `EncounterSeed` y `EntitySeed`.
- Una misma seed debe reconstruir el estado inicial.
- Guardar sólo la seed, versión del generador y deltas persistentes cuando sea viable.
- Versionar los algoritmos de generación para proteger partidas guardadas.
- Validar rangos científicos y jugables antes de aceptar un resultado.

## 10. Ciencia

- Usar nombres y unidades explícitos: `PressureKPa`, `TemperatureKelvin`, `GravityMS2`.
- Centralizar conversiones.
- Evitar números mágicos.
- Diferenciar simulación, aproximación y ficción especulativa.
- Explicar supuestos en comentarios y documentación, no mediante tutoriales invasivos.
- Presentar consecuencias jugables antes que fórmulas al jugador.

## 11. Persistencia

El guardado debe registrar:

- Versión de savegame.
- Seed raíz y versión del generador.
- Estado del jugador y del traje.
- Descubrimientos de bitácora.
- Mapa revelado.
- Inventario mínimo.
- Objetivos y progreso.
- Deltas realizados sobre el mundo.

Toda migración de formato requiere prueba automática.

## 12. Git y seguridad del trabajo

- Preservar cambios ajenos.
- No utilizar comandos destructivos sobre el repositorio o directorios amplios.
- No realizar force push.
- No confirmar secretos, tokens, cachés, builds ni archivos temporales.
- Utilizar Git LFS para binarios grandes que deban versionarse.
- Commits pequeños con formato: `tipo(sistema): resultado`.
- Tipos sugeridos: `feat`, `fix`, `test`, `docs`, `refactor`, `perf`, `build`.
- Etiquetar builds jugables por hito.

## 13. Documentación viva

Actualizar durante el trabajo:

- `Docs/BACKLOG.md`: tareas y estado.
- `Docs/DEVELOPMENT_STATE.md`: qué funciona, última prueba y siguiente paso.
- `Docs/DECISIONS.md`: decisiones menores.
- `Docs/ADR/`: decisiones arquitectónicas importantes.
- `Docs/TEST_REPORT.md`: comandos, resultados y fecha.
- `Docs/KNOWN_ISSUES.md`: fallos reproducibles y severidad.

La documentación debe reflejar el estado real. No declarar una función terminada sin evidencia.

## 14. Criterio de bloqueo

Antes de pedir ayuda:

1. Reproducir el problema.
2. Reducirlo a su causa probable.
3. Leer logs completos relevantes.
4. Probar tres alternativas técnicamente diferentes.
5. Verificar que ninguna viole alcance o integridad.
6. Documentar intentos y evidencias.

La consulta debe incluir: objetivo, comportamiento observado, comportamiento esperado, error exacto, pruebas realizadas y recomendación preferida.

## 15. Condición de finalización

El agente no puede declarar terminado el MVP hasta que:

- Todos los criterios obligatorios estén implementados.
- El proyecto compile desde un checkout limpio con las dependencias documentadas.
- Pasen las pruebas automáticas.
- El recorrido principal pueda completarse de principio a fin.
- Guardar, cerrar, abrir y continuar funcione.
- Exista un ejecutable Development para Windows.
- Exista un informe final con controles, funcionalidades, rendimiento, limitaciones y errores conocidos.
- El alcance excluido continúe excluido.

Al finalizar, detener la expansión y solicitar una única sesión de prueba humana.
