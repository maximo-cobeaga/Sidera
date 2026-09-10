# Índice de documentación — ASTRAEON

Qué es cada archivo, cuánta autoridad tiene y cuándo se toca. Fecha: 2026-09-09.

**Cómo leer este índice.** La columna que importa es **Autoridad**:

| Marca | Significado |
|---|---|
| **RECTOR** | Manda. Un conflicto con cualquier otro documento lo gana este |
| **VIVO** | Se actualiza en cada sesión. Refleja el estado real, no la intención |
| **CONTRATO** | Regla técnica vigente sobre un sistema concreto |
| **REFERENCIA** | Alcance y diseño. No se contradice, pero no ordena el trabajo del día |
| **HISTÓRICO** | Describe algo ya hecho. Se lee para entender por qué, no para decidir qué |
| ⚠ **DESACTUALIZADO** | Contiene dirección que el ADR 0004 anuló. **No seguir sin contrastar** |

---

## 1. Lo que hay que leer antes de tocar nada

En este orden, según `AGENTS.md` §2.

| Archivo | Autoridad | Qué es |
|---|---|---|
| `AGENTS.md` (raíz) | **RECTOR** | Contrato operativo de agentes. Misión, fuentes de verdad, restricciones técnicas y planetarias, límites de la fase, ciclo de trabajo, estrategia de pruebas, presupuesto de rendimiento, persistencia y condición de cierre. **Es el primer archivo que se lee siempre** |
| `Docs/ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` | **RECTOR** | Plan maestro de transición al juego objetivo. Define las 10 fases, los tres productos (A núcleo planetario, B vertical slice esférico, C juego v1), la arquitectura planetaria, el pipeline Higgsfield → Blender → Unreal y los contratos por categoría de asset. 1.941 líneas. **Es el documento que fijó la dirección actual** |
| `Docs/PLAN_TRANSICION_EJECUCION.md` | **RECTOR** | Traduce el anterior a trabajo ordenado contra el código real: matriz de migración archivo por archivo, qué se conserva y qué se adapta, cuarentena de pruebas, deudas diferidas a propósito y riesgos del plan |
| `Docs/PHASE_STATUS.md` | **RECTOR** / **VIVO** | Fase activa, su puerta de salida marcada punto por punto, y la tabla de cuarentena. **El archivo que dice qué está permitido hacer hoy** |
| `Docs/ADR/` | **RECTOR** | Decisiones arquitectónicas. Ver §2 |

---

## 2. ADR — decisiones arquitectónicas

Una decisión que cambia la forma del proyecto va aquí, no en `DECISIONS.md`. En conflicto gana
el más reciente.

| Archivo | Estado | Qué decidió |
|---|---|---|
| `ADR/0001-bootstrap-cpp-unreal-project.md` | vigente | Arranque del proyecto C++ en Unreal |
| `ADR/0002-continuous-terrain-prototype.md` | vigente | Prototipo de malla continua como experimento aislado |
| `ADR/0003-fixed-region-world-architecture.md` | **parcialmente superado** | Mundo fijo: la estructura no deriva de `WorldSeed`, se consumen regiones diseñadas. *Sobrevive el principio; el 0004 anuló su orden de prioridades* |
| `ADR/0005-cuarentena-surfacecontract-a-fase-3.md` | **vigente, el más nuevo** | `Terrain.SurfaceContract` vuelve en la Fase 3: valida contenido de región, no núcleo planetario. `Terrain.Relief` sí volvió en la Fase 1 |
| `ADR/0004-planetas-esfericos-fundacionales.md` | vigente | La esfera y la gravedad radial pasan a requisito fundacional. Corte inmediato del mundo plano, cuarentena de pruebas, estado mutable como prerrequisito de la Fase 2 |

---

## 3. Estado vivo — se actualiza en cada sesión

Obligatorio actualizarlos al trabajar (`AGENTS.md` §13). Si divergen de la realidad, la realidad
gana y el documento está mal.

| Archivo | Qué es | Cuándo se toca |
|---|---|---|
| `DEVELOPMENT_STATE.md` | Qué funciona, última prueba y siguiente paso. Diario cronológico inverso, 484 líneas | Al cerrar cualquier trabajo |
| `BACKLOG.md` | Tareas con estado `[ ] [~] [x]`, agrupadas por bloque | Al abrir o cerrar una tarea |
| `TEST_REPORT.md` | Comandos ejecutados, resultados y fecha. 835 líneas | Cada vez que se corren pruebas |
| `KNOWN_ISSUES.md` | Fallos reproducibles con motivo, impacto y cierre esperado. Incluye los resueltos, marcados | Al encontrar o cerrar un fallo |
| `DECISIONS.md` | Decisiones menores con su porqué. Las mayores van a `ADR/` | Al elegir entre alternativas |

---

## 4. Alcance y diseño

| Archivo | Autoridad | Qué es |
|---|---|---|
| `GAME_DESIGN_MASTER.md` | **REFERENCIA** ⚠ | Documento maestro del juego: mundo, tono, sistemas, ficción y visión completa. 743 líneas. *Sitúa el sistema estelar en una fase posterior; el ADR 0004 adelantó coordenadas, esfera y gravedad* |
| `MVP_LA_PRIMERA_SENAL.md` | **REFERENCIA** ⚠ | Alcance jugable del vertical slice: criterios de aceptación, bucle, contenido y condición de terminado. Sigue siendo el alcance de la **Fase 3**, ahora sobre la esfera. *Asume que la región puede existir sin planeta completo* |
| `ASTRAEON_PLAN_DESARROLLO_MAPA.md` (raíz) | **REFERENCIA** ⚠ | Plan técnico del mapa dinámico en 10 fases. 1.188 líneas. *Ordena región plana primero, planeta después; el ADR 0004 lo invirtió* |
| `VISUAL_LANGUAGE.md` | **CONTRATO** | Lenguaje visual: reglas de producción del blockout derivadas del tono. 20 líneas, denso |
| `graphics/ASTRAEON_DESIGN_BRIEF.md` | **REFERENCIA** | Brief de dirección de arte |

---

## 5. Contratos técnicos

Reglas vigentes sobre un sistema concreto. Se consultan antes de tocar ese sistema.

| Archivo | Qué contrata |
|---|---|
| `PROCEDURAL_TERRAIN_CONTRACT.md` | El terreno se genera por seed, pero la seed no autoriza resultados incoherentes: plataforma, salida, rutas, POIs y conectividad se garantizan **antes** de materializar. Es el contrato que cerró el Bloque A |
| `BLENDER_PIPELINE.md` | Pipeline Blender → validación → FBX → Unreal, generado desde código + JSON |
| `ART_ASSET_MASTER_PLAN.md` | Familias de assets y contrato de calidad Q1 |
| `MODULAR_ASSET_GENERATION.md` | Catálogos y selección de módulos como datos separados del gameplay |
| `GUIA_ARTE_PLANETAS_BLENDER.md` | ⚠ Pipeline de arte para planetas procedurales. *Sus radios de 150 m a 2 km dejaron de ser tiers objetivo: el ADR 0004 los reclasificó como laboratorio. El resto —cube-sphere, materiales tileables por bioma, Blender no modela el planeta final— sigue vigente y **anticipó** la dirección actual* |
| `CAMARA_Y_MANOS.md` | Cámara en tercera persona y manos de primera persona: qué está hecho y qué falta |
| `evidencia/BASELINE_RENDIMIENTO.md` | El punto de comparación de rendimiento anterior a la transición, cómo reproducirlo y **qué NO mide** |
| `evidencia/CALIBRACION_BLENDER_UNREAL.md` | Que un metro de Blender llega como 100 cm, con números, y el preset `UE57_AST_V1` congelado |

---

## 6. Informes de integración — HISTÓRICO

Describen **qué arte se integró y cómo quedó enganchado**. Se leen para entender un sistema
existente antes de modificarlo, no para decidir el trabajo del día.

| Archivo | Qué integró |
|---|---|
| `ITACA_INTEGRATION.md` | Interior de Ítaca Q1 y su ensamblado en los mapas |
| `ITACA_EXTERIOR_INTEGRATION.md` | Casco, motores, patines, antena, consola y fabricador |
| `REGION_ART_INTEGRATION.md` | Siete mallas para recursos y puntos de interés |
| `CREATURE_ART_INTEGRATION.md` | `Umbra Grazer`: modelo, rig y estados |
| `CHARACTER_TOOLS_INTEGRATION.md` | Personaje y herramientas como paquetes de `Content/` |
| `HUMANOID_ART.md` | Primer lote de humano, manos y siete animaciones |
| `AUDITORIA_ARTE.md` | Cuánto del arte generado llega de verdad al juego. Levantó el defecto de los 45 clips que no se reproducían |

---

## 7. Protagonista — pipeline propio

El personaje principal tiene documentación separada porque su pipeline es el más largo del proyecto.

| Archivo | Qué es |
|---|---|
| `PENDIENTE_PROTAGONISTA.md` | Estado real y pendientes del protagonista. 321 líneas. Incluye la medición de las poses de brazo en cruz |
| `INVESTIGACION_PERSONAJE_INVISIBLE.md` | **RESUELTO.** Por qué el protagonista no se veía: la importación FBX perdía la escala 100 del Armature |
| `ANIMACION_PROTAGONISTA_CORRECCION_20260910.md` | Registro de diagnóstico, corrección Blender/Unreal, evidencias, pruebas y procedimiento de prueba humana |
| `graphics/characters/main_player/docs/MAIN_CHARACTER_PIPELINE.md` | Contrato y evidencia del pipeline del personaje |
| `graphics/characters/main_player/docs/SCENE_PASSPORT.md` | Pasaporte de la escena Blender |
| `graphics/characters/main_player/docs/COMPLETION_PASSPORT.md` | Cierre del lote |

---

## 8. Handoffs — cierre de sesión, HISTÓRICO

Cada uno captura el estado al cerrar un tramo de trabajo: qué quedó verde, qué falta probar a
mano y por dónde seguir. El más reciente es el útil; los anteriores son antecedente.

| Archivo | Cierra |
|---|---|
| `HANDOFF_SESION_20260910_FASE1_CERRADA.md` | **El más reciente.** Cierre de la Fase 1: qué leer al reanudar, cómo verificar, el defecto de colisión que parecía animación, y la Fase 2 con su deuda heredada |
| `HANDOFF_SESION_20260909_TRANSICION.md` | Transición a planetas esféricos: 12 commits, qué leer al reanudar, cómo verificar, siete trampas medidas y la ruta de la Fase 1 |
| `HANDOFF_SESION_20260909.md` | Cierre del arte del protagonista, anterior a la transición |
| `HANDOFF_PERSONAJE_PRINCIPAL.md` | Marcado **SUPERADO** en su propia cabecera |
| `HANDOFF_FASE_MUNDO_VIVO.md` | Decisión de salir del MVP e Ítaca como nave |
| `HANDOFF_ESCOTILLA_INTERACCION.md` | Bloqueante de la escotilla que no respondía |

---

## 9. Superados por la transición ⚠

Describen el mundo plano. **Ninguno se borra** —explican por qué el código es como es— pero
ninguno ordena trabajo nuevo.

| Archivo | Qué decía | Qué lo superó |
|---|---|---|
| `ESTADO_Y_RUTA_MAPA.md` | Contraste del repo contra el plan del mapa y ruta en tres bloques | El Bloque A sigue válido y cerrado. El Bloque B quedó absorbido por la Fase 2; el Bloque C, cancelado |
| `MVP_WORLD_ARCHITECTURE.md` | Arquitectura de mundo y mapas del MVP plano | ADR 0004 |
| `WORLD_ARCHITECTURE_AUDIT.md` | Clasificación del código contra la decisión anterior | Reemplazado por la matriz de migración del plan de ejecución §2 |
| `PLAN_TERRENO_REGIONAL.md` | Plan T00–T06 de terreno regional | Su T01 (malla continua) revive como constructor de patch en la Fase 2 |
| `DISENO_RECORRIDO_REGIONAL.md` | Layout diseñado de Region A | Se reproyecta sobre la esfera en la Fase 3 |
| `ASSETS_PENDIENTES_DISENO_ANIMACION.md` | Componentes pendientes de diseño y animación | Su propia cabecera ya avisa que §3.2 y §3.3 no describen el estado actual |

---

## 10. Entorno, prompts y duplicados

| Archivo | Qué es |
|---|---|
| `README.md` (raíz) | Presentación del repositorio |
| `WINDOWS_11_SETUP.md` | Preparación del entorno: hardware de referencia, motor, toolchain |
| `PROMPT_INICIAL_DESARROLLO.md` (raíz) | Prompt de arranque de desarrollo autónomo. **Anterior a la transición**: el prompt vigente está en `ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` §11.5 |
| `Prompt maestro — Planificación e inicio del pipeline de arte de ASTRAEON.md` (raíz) | Prompt en inglés del pipeline de arte. 1.012 líneas. Histórico |
| `Docs/evidencia/` | Capturas de QA referenciadas por los informes |

**Duplicados raíz ↔ `Docs/`.** `GAME_DESIGN_MASTER.md`, `MVP_LA_PRIMERA_SENAL.md` y
`WINDOWS_11_SETUP.md` existen idénticos en los dos sitios. Son copias congeladas y no han
divergido. `ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` **no** se duplicó a propósito: es un
documento vivo y dos copias derivarían.

---

## 11. Dónde escribir según lo que estés haciendo

| Situación | Archivo |
|---|---|
| Terminaste una tarea | `DEVELOPMENT_STATE.md` + `BACKLOG.md` |
| Corriste pruebas | `TEST_REPORT.md` |
| Encontraste un fallo | `KNOWN_ISSUES.md` |
| Elegiste entre dos opciones técnicas | `DECISIONS.md` |
| La elección cambia la forma del proyecto | `ADR/` nuevo |
| Abriste o cerraste una fase | `PHASE_STATUS.md` |
| Integraste arte nuevo | Informe propio en §6 + `AUDITORIA_ARTE.md` |
| Cerrás la sesión | `HANDOFF_SESION_<fecha>.md` |

---

*Las descripciones de los documentos que no se leyeron completos provienen de su propia
declaración de apertura. Si un archivo cambió de propósito sin cambiar su cabecera, este índice
lo hereda: al detectarlo, corregir ambos.*
