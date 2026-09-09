# Contrato operativo de agentes — ASTRAEON

Este archivo contiene instrucciones permanentes para cualquier agente que trabaje en este repositorio. Tiene prioridad sobre instrucciones improvisadas, salvo orden explícita del propietario.

## 1. Misión

Construir y verificar **ASTRAEON: Legado del Vacío** en Unreal Engine 5.7 para Windows 11,
por fases, según `Docs/ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md`.

La experiencia es una aventura espacial científica en primera persona. El núcleo jugable es:

> explorar → medir → comprender → actuar → registrar

La ciencia debe funcionar como herramienta y ventaja, nunca como examen escolar.

**La misión inmediata no es el MVP plano.** Desde el 2026-09-09 ([ADR 0004](Docs/ADR/0004-planetas-esfericos-fundacionales.md))
el orden es: primero el núcleo planetario esférico mínimo que demuestre la promesa, y después
**La primera señal** reconstruida sobre él. El vertical slice plano existente se conserva como
build etiquetada, no como base de trabajo.

La fase activa y su puerta de salida están en `Docs/PHASE_STATUS.md`. **No trabajar sobre fases
futuras si la puerta de la fase actual no pasó.**

## 2. Fuentes de verdad

Antes de actuar, leer en este orden:

1. `AGENTS.md`
2. `Docs/ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md`
3. `Docs/PLAN_TRANSICION_EJECUCION.md`
4. `Docs/PHASE_STATUS.md`
5. `Docs/DEVELOPMENT_STATE.md`, si existe
6. `Docs/BACKLOG.md`, si existe
7. `Docs/MVP_LA_PRIMERA_SENAL.md` — alcance jugable, ahora destino de la Fase 3
8. `Docs/GAME_DESIGN_MASTER.md`
9. ADR relevantes dentro de `Docs/ADR/`, empezando por 0004

En caso de conflicto:

1. Seguridad e integridad del repositorio.
2. ADR aceptados, del más reciente al más antiguo.
3. Alcance y puerta de salida de la fase activa.
4. Contrato operativo de este archivo.
5. Documento maestro del juego.
6. Preferencias de implementación registradas posteriormente.

No reinterpretar una idea del juego completo como requisito inmediato de la fase activa.

**Si queda una contradicción entre documentos rectores, detener la implementación y resolver la
documentación primero.** Varios documentos anteriores al 2026-09-09 —`MVP_LA_PRIMERA_SENAL.md`,
`GAME_DESIGN_MASTER.md`, `ASTRAEON_PLAN_DESARROLLO_MAPA.md`, `GUIA_ARTE_PLANETAS_BLENDER.md`—
todavía describen un mundo plano. Donde contradigan al ADR 0004, gana el ADR.

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
- Se requiera ampliar el alcance de la fase activa.
- Un bloqueo persista después de tres estrategias técnicamente distintas y documentadas.

## 4. Restricciones técnicas

- Motor: Unreal Engine 5.7.4, versión fijada durante toda la transición planetaria.
- Plataforma objetivo: Windows 11 x64.
- Núcleo del gameplay: C++.
- Blueprints: presentación, composición visual y configuración delgada.
- Enhanced Input para controles.
- Un solo jugador y funcionamiento offline.
- Generación procedural determinista mediante seeds explícitas.
- Unidades físicas documentadas y, preferentemente, SI internamente.
- Datos de contenido separados de la lógica mediante estructuras, Data Assets o tablas cuando resulte apropiado.
- Sistemas desacoplados mediante componentes, subsistemas e interfaces.
- No introducir servicios de IA en tiempo de ejecución. Higgsfield es herramienta de producción, nunca dependencia de runtime.
- No introducir plugins pagos.
- No descargar ni redistribuir assets sin licencia comprobable.
- No actualizar Unreal Engine, toolchain, Blender, Higgsfield ni dependencias durante una producción activa sin un ADR aprobado.

La lógica imprescindible no puede existir exclusivamente en un `.uasset` binario si puede representarse de forma mantenible en código o configuración textual.

### 4.1 Restricciones planetarias (ADR 0004)

Obligatorias para todo código nuevo, sin excepción y sin fase de gracia:

- **Ningún sistema nuevo puede asumir que el eje Z global representa arriba.** El vector arriba
  local es `Normalize(ActorPosition - PlanetCenter)`.
- **El terreno final no puede basarse en Unreal Landscape plano.** Una superficie plana no es un
  fallback de producción aceptable.
- El tamaño de un planeta es un **dato**, nunca un `Scale` de Blueprint ni una malla estática única.
- La altura de la superficie se consulta por **dirección planetaria global**, nunca por UV local
  aislada: es lo único que garantiza que dos caras coincidan en su borde.
- La identidad persistente de un objeto **no** es su `Transform` mundial: es `BodyId` + dirección
  superficial + altitud + coordenadas locales.
- Posiciones astronómicas y planetarias persistentes en `double`; física, animación y render
  cercano en el frame local.
- Separar siempre **definición** (hechos reproducibles), **representación** (lo visible según
  distancia) y **estado mutable** (deltas de la partida).

Una ciudad o estructura puede diseñarse en un plano tangente, pero cada elemento debe proyectarse
y orientarse sobre la esfera.

## 5. Límites de la fase activa

Implementar únicamente los entregables de la fase activa según `Docs/PLAN_TRANSICION_EJECUCION.md`
§4 y `Docs/PHASE_STATUS.md`. El alcance jugable de la Fase 3 sigue siendo el de
`Docs/MVP_LA_PRIMERA_SENAL.md`, reconstruido sobre el núcleo planetario.

Está prohibido incorporar mientras no lo habilite una fase:

- Galaxia completa o universo infinito.
- **Contenido planetario masivo**: biomas poblados, vegetación y props a escala de planeta, o
  colisión y actores de gameplay para toda la superficie.
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

> **Corregido el 2026-09-09 por el ADR 0004.** Este apartado prohibía "planetas esféricos
> totalmente transitables". La esfera transitable pasó a ser **fundacional**: lo que queda fuera
> es el contenido masivo que la puebla, no la geometría ni la gravedad radial. Ninguna otra
> prohibición de esta lista cambió.

Se permite crear interfaces o estructuras mínimas que faciliten expansiones futuras, pero no implementar las expansiones.

Además, un agente **no debe**:

- Generar un lote completo de assets antes de validar una muestra.
- Declarar una fase terminada sin evidencia contra su puerta de salida.
- Incorporar un asset desde `HF_Incoming` directamente, sin pasar por Blender, validación,
  manifiesto, exportación e importación.
- Cambiar el esqueleto de una familia ya aprobada sin un ADR.
- Introducir un plugin pago o un servicio en tiempo de ejecución.
- Ocultar fallos con defaults silenciosos.

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

**Única excepción vigente: la cuarentena de la transición planetaria.** El corte del mundo plano
deja ~15 pruebas sin el mundo sobre el que corrían. Están listadas una por una en
`Docs/PLAN_TRANSICION_EJECUCION.md` §3.1 con la fase en que vuelven. Reglas:

- No se borra ninguna, y ninguna se relaja para que apruebe.
- Nada entra en cuarentena sin quedar en esa tabla con fase de retorno.
- **Una fase no cierra con pruebas suyas todavía en cuarentena.** Que la tabla quede vacía es la
  puerta de salida de la Fase 3.

Si al migrar una prueba hay que debilitar lo que exige para que pase, el defecto está en la
migración, no en la prueba. Vale sobre todo para `Astraeon.WorldGen.Terrain.TraversalDetectsWalls`,
cuya razón de existir es **exigir que el validador falle** cuando la región no se puede recorrer.

## 8. Presupuesto de rendimiento

Objetivos provisionales, ajustables **sólo con profiling registrado** o mediante ADR.

- Resolución de referencia: 1920×1080. Frame budget total: 16,67 ms.
- Hardware de referencia: i5-14400F, RTX 4060 8 GB, 16–32 GB RAM, SSD.

| Métrica | Objetivo |
|---|---|
| FPS medio | 60 o más |
| Percentil 1% | 45 FPS o más |
| Hitch visible | Ningún pico recurrente superior a 50 ms |
| VRAM | Margen estable en una GPU de 8 GB |
| Memoria | Sin crecimiento ilimitado en una sesión de 60 minutos |
| Generación | Fuera del game thread siempre que sea posible |
| Inicio del build | Menos de 45 segundos en SSD |

Medir percentiles y picos, no sólo el promedio: el fallo característico del streaming planetario
es un hitch que un promedio sano esconde. No aceptar stutter recurrente durante movimiento rápido
ni durante un cambio de LOD.

No generar colisión, vegetación ni actores de gameplay para todo el planeta.

Primero se mide; después se optimiza.

## 9. Diseño procedural

Toda aleatoriedad jugable debe provenir de streams deterministas derivados de una seed raíz. No utilizar aleatoriedad global no controlada para contenido persistente.

Principios:

- Generar mediante causas, no combinaciones cosméticas independientes.
- Separar `WorldSeed`, `SystemSeed`, `BodySeed`, `RegionSeed`, `PatchSeed`, `EncounterSeed` y `EntitySeed`.
- Una misma seed debe reconstruir el estado inicial.
- Guardar sólo la seed, versión del generador y deltas persistentes cuando sea viable.
- Versionar los algoritmos de generación para proteger partidas guardadas.
- Validar rangos científicos y jugables antes de aceptar un resultado.

Con generación en workers, además:

- Derivar cada seed por hash estable de sus entradas, con canal de generación y versión
  explícitos. El resultado **no puede depender del orden en que terminen las tareas asíncronas**.
- Cada módulo recibe entradas explícitas, usa su propia seed derivada, devuelve datos y puede
  probarse sin cargar el mundo completo. Cada uno tiene su validador.
- Un resultado con revisión vieja no debe sobrescribir un patch nuevo; una solicitud obsoleta
  debe poder cancelarse.

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
- **Ubicación planetaria**: `BodyId`, dirección superficial, altitud y coordenadas locales.
- Estado del jugador y del traje.
- Descubrimientos de bitácora.
- Mapa revelado.
- Inventario mínimo.
- Objetivos y progreso.
- Deltas persistentes realizados sobre el mundo.

**Un `Transform` mundial no es una ubicación persistente válida** (ADR 0004): con marcos locales
y patches que se descargan, deja de significar lo mismo entre sesiones.

El guardado persiste seed raíz + versiones de generadores + estado del jugador + deltas. No
persiste geometría base ni millones de instancias derivables de la seed.

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

- `Docs/PHASE_STATUS.md`: fase activa, puerta de salida y cuarentena de pruebas.
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

### 15.1 Cierre de una fase

Una fase termina **por evidencia contra su puerta de salida**, nunca por cantidad de código ni
por compilar. Cada cierre necesita, según `ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` §10.4:

- Commit o diff identificable.
- Comando de build.
- Resultado de las pruebas.
- Captura o video.
- Perfil de rendimiento.
- Lista de warnings nuevos.
- Limitaciones conocidas y próximo paso.
- **Ninguna prueba propia de la fase en cuarentena** (§7).

Una captura no reemplaza una prueba, y una prueba verde no reemplaza la inspección visual.
Actualizar `Docs/PHASE_STATUS.md` al abrir y al cerrar cada fase.

### 15.2 Cierre del vertical slice (Fase 3)

El agente no puede declarar terminado el vertical slice esférico hasta que:

- Todos los criterios jugables de `Docs/MVP_LA_PRIMERA_SENAL.md` pasen **sobre la esfera**.
- La tabla de cuarentena de `PLAN_TRANSICION_EJECUCION.md` §3.1 quede vacía.
- El proyecto compile desde un checkout limpio con las dependencias documentadas.
- Pasen las pruebas automáticas.
- El recorrido principal pueda completarse de principio a fin.
- Guardar, cerrar, abrir y continuar funcione, y la misma seed conserve terreno y contenido.
- No exista dependencia funcional de una superficie plana.
- Exista un ejecutable Development para Windows dentro del presupuesto de rendimiento.
- Exista un informe final con controles, funcionalidades, rendimiento, limitaciones y errores conocidos.
- El alcance excluido continúe excluido.

Al finalizar, detener la expansión y solicitar una única sesión de prueba humana.
