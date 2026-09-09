# Estado del proyecto contra el plan del mapa, y ruta al juego objetivo

Fecha: 2026-09-08. Contrasta el estado real del repositorio con
`ASTRAEON_PLAN_DESARROLLO_MAPA.md` (10 fases). No introduce alcance nuevo: ordena el que ya
está acordado.

**Veredicto.** El proyecto está en las **fases 1–2** del plan, con la **fase 10
sobrecumplida** y la **fase 6 adelantada de orden**. La brecha crítica no es de contenido:
es de **datos y estado**.

---

## 1. Dónde estamos, medido

| | |
|---|---|
| Código | 11.296 líneas C++, 34 headers, 32 `.cpp` |
| Pruebas | 55 automáticas en verde, smoke del recorrido crítico y packaging verificado |
| Mundo | 1 planeta, 1 región (500 × 500 m), **1 bioma**, 1 mapa (`L_AstraeonBootstrap`), sin World Partition |
| Bucle jugable | explorar → medir → decidir protección → recolectar → fabricar → construir → cazar/comer → resolver señal → guardar/cargar. Completo y probado |
| Arte | personaje, criatura, Ítaca interior/exterior, kit regional y herramientas: todo Q1 propio e integrado |
| Subsystems / Data Assets | **0 y 0** (`grep` sobre `Source/` y `Content/Astraeon/`) |

---

## 2. Contraste fase por fase

| Fase del plan | Estado | Qué falta de verdad |
|---|---|---|
| **0 — Contratos y seeds** | Parcial | `UAstraeonWorldGenerator::DeriveSeed(root, contexto)` usa clave versionada + CRC32 y hay `GeneratorVersion`: cumple §2.2 **bien**. Pero los perfiles están *hardcodeados* en `AstraeonWorldProfiles.cpp`, no en datos. El criterio de salida "los datos pueden crearse sin modificar la lógica" **no se cumple** |
| **1 — Región jugable** | Casi, bloqueada en A03 | Falta la **superficie diseñada** (P0, pendiente desde el 2026-09-06) y las **celdas lógicas** con `RegionId`, bounds y estado de generación. No hay prueba de **conectividad** de la ruta crítica: `Astraeon.WorldGen.Terrain.Relief` valida el escalón caminable, no que exista camino. Los 30–45 min del criterio nunca se midieron |
| **2 — Biomas y recursos** | Parcial | Recursos deterministas ✓ y seeds secundarias declaradas ✓ (`FAstraeonSecondaryContentSeeds`). Pero `EAstraeonBiomeId` tiene **un solo valor** y no existe `BiomeProfile` con vegetación, rocas, fauna, densidad y exclusiones |
| **3 — Civilizaciones** | Sin empezar | Correcto: el plan la sitúa después |
| **4 — Historia emergente** | Sin empezar | Correcto |
| **5 — Ciudades** | Sin empezar | Correcto |
| **6 — Assets modulares** | En curso, **fuera de orden** | El plan §15 la pone en el paso 6, después de streaming. El contrato de asset y las familias existen (`ART_ASSET_MASTER_PLAN.md`), pero se está produciendo antes de cerrar las fases 1–2 |
| **7 — Streaming** | Inexistente | Ni sectores, ni estados `Unloaded → Active`, ni World Partition. `MaterializeCurrentRegion()` **rehace la región entera** al aterrizar |
| **8 — Sistema planetario** | Sólo el dato | Un `PlanetProfile` fijo. Sin representaciones por distancia ni jerarquía de coordenadas |
| **9 — Viaje superficie-espacio** | Prototipo | Vuelo atmosférico con techo narrativo de 240 m. Sin la máquina de estados `Docked → … → Landing` |
| **10 — Validación y calidad** | **Por encima de lo pedido** | 55 tests, determinismo, migración de saves v1→v2, smoke sobre ejecutable, `-AstraeonDiag`. Falta sólo el **presupuesto de rendimiento explícito** (ms/frame, draw calls, memoria) |

---

## 3. Primer gran hito del plan (§17)

| Requisito | Estado |
|---|---|
| Se genera a partir de una seed | ✓ `Astraeon.WorldGen.Region.Determinism` |
| Tiene un recorrido jugable | ✓ `Astraeon.Functional.CriticalPath.FullFlow` |
| Contiene biomas y recursos | **Parcial**: recursos sí; biomas, uno solo |
| Tiene una criatura escaneable | ✓ `Astraeon.Creatures.Scanning.LogbookEntry` |
| Incluye una ruina o señal | ✓ `Astraeon.Narrative.Signal.ResolveObjective` |
| Permite medir variables ambientales | ✓ `Astraeon.Science.Environment.Breathability` |
| Permite fabricar una solución | ✓ `Astraeon.Resources.Crafting.SignalResonator` |
| Registra descubrimientos en bitácora | ✓ `Astraeon.Knowledge.Logbook.Upsert` |
| Se puede guardar y cargar | ✓ `Astraeon.Persistence.SaveGame.RoundTrip` |
| Se puede regenerar idénticamente | ✓ `Astraeon.WorldGen.Environment.Determinism` |
| No depende de planetas ni civilizaciones | ✓ |

Diez de once. **Falta "biomas" en plural** y medir el recorrido. Está a un paso de poder
declararse cerrado.

---

## 4. Las tres deudas que sí importan

### 4.1 Datos dentro de la lógica

El plan abre con esto (§2.1) y su regla final (§19) lo repite. Hoy, añadir un bioma, un
recurso o una región exige recompilar. Con una sola región no se nota; desde la fase 3 cada
civilización, ciudad y evento multiplica el coste. Es la deuda **más barata de pagar ahora**
y la más cara de pagar después.

### 4.2 No existe `RuntimeStateManager`

El plan separa mundo base (seed) de estado mutable (§2.4). El proyecto persiste con listas
ad-hoc dentro del `SaveGame`, y el síntoma ya está documentado: **las criaturas muertas
reaparecen al rematerializar la región** (`BACKLOG.md`, MV4). Ese mismo agujero, con
streaming encima, se convierte en objetos duplicados y progreso perdido — el riesgo §18
"streaming inestable".

### 4.3 Un solo mapa, sin sectores

Ni la segunda región ni la primera ciudad caben sin esto, y reconstruir la región entera de
golpe no escala.

---

## 5. El riesgo real, hoy

No es alcance excesivo hacia el futuro: es **pulido prematuro del presente**. El arte del
protagonista consumió las últimas sesiones, y su continuación natural —pulir animaciones—
consumiría más, mientras **A03 lleva días parada** y el resto del plan de terreno (T01–T06 en
`PLAN_TERRENO_REGIONAL.md`) depende de ella.

El plan dice literalmente que no se debe avanzar de fase sólo porque el código compile. El
converso también aplica: **no se debe pulir una fase que todavía no cerró.**

---

## 6. Ruta recomendada

### Bloque A — cerrar la fase 1 (el desbloqueo)

Implementar el terreno procedural coherente ya especificado en
`PROCEDURAL_TERRAIN_CONTRACT.md`: sampler con zonas de garantía, invariantes de pendiente y
altura libre, malla continua. Añadir la **prueba de conectividad de la ruta crítica** que el
plan §1.3 exige y que hoy no existe.

*Criterio de salida:* cinco seeds recorren Ítaca → recursos → señal sin bloqueo, verificado
en el ejecutable empaquetado, y el recorrido cronometrado contra los 30–45 min.

### Bloque B — pagar la deuda de datos y estado

Mover `PlanetProfile` y `RegionProfile` a Data Assets, añadir `BiomeProfile` y un **segundo
bioma**, e introducir el estado mutable como capa explícita sobre la seed.

*Criterio de salida:* se añade un bioma sin tocar C++, y una criatura abatida sigue abatida
después de despegar y aterrizar.

### Bloque C — sectores y segunda región

Celdas lógicas con estado de generación, streaming de sectores pequeños dentro de Region A y
después una Region B que reutilice la arquitectura sin duplicar lógica. Eso abre el segundo
gran hito del plan: una ciudad de una civilización con historia local.

### El arte, mientras tanto

Se mantiene en **Q1**. El pulido de animación registrado en `KNOWN_ISSUES.md` entra como
tarea acotada dentro de un bloque, no como fase propia.

### Lo que no se toca todavía

Civilizaciones, historia, ciudades, catálogo planetario y viaje espacial. El propio
`MVP_WORLD_ARCHITECTURE.md` los condiciona a que Region A sea reutilizable, y el plan del
mapa los ordena después de streaming.

---

## 7. Deuda menor, deliberadamente no pagada

La estructura de carpetas que propone el plan (§0.2: `World/Seed`, `World/Galaxy`,
`Civilization/`, `History/`, `Settlements/`…) no coincide con la actual (`WorldGen/`,
`Survival/`, `Knowledge/`, `Presentation/`…). Renombrar hoy sería churn sin beneficio: la
decisión se toma cuando exista el primer sistema de civilización que necesite su carpeta.
