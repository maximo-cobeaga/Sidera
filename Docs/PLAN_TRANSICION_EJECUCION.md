# Plan de ejecución de la transición al juego objetivo

Fecha: 2026-09-09. Convierte `ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` en trabajo ordenado y
verificable contra el estado real del repositorio. **No introduce alcance nuevo**: ordena el que
ese documento ya fijó, y registra dónde el código actual lo contradice.

Sustituye la ruta de `ESTADO_Y_RUTA_MAPA.md` §6 a partir del Bloque B. El Bloque A sigue cerrado
y su resultado se conserva. El Bloque C queda **superado**: los sectores planos no se construyen.

---

## 0. Decisiones tomadas al abrir este plan

| Decisión | Elegida | Consecuencia |
|---|---|---|
| Mundo plano durante la migración | **Corte inmediato** | `L_AstraeonBootstrap` se retira. No hay build jugable nueva hasta la Fase 3. Obliga a la cuarentena de pruebas de §3.1 y hace de la build etiquetada de §3.2 la única demostrable |
| Orden de arranque | **Fase 0 documental y spike técnico en paralelo** | La Fase 0 cierra con el spike como evidencia adjunta, no sólo con documentos sincronizados |

La segunda decisión no rompe el orden obligatorio del documento rector: la Fase 1 no empieza hasta
que la puerta de la Fase 0 pase con ambas mitades.

---

## 1. Punto de partida, medido

| | |
|---|---|
| Código | 12.233 líneas C++, 34 headers, 38 `.cpp` |
| Pruebas | 58 automáticas en verde; smoke del recorrido crítico y empaquetado verificados |
| Mundo | 1 planeta, 1 región de 500 × 500 m, 1 bioma, 1 mapa, sin World Partition |
| Gravedad | **No existe como sistema.** `GravityMS2` es un escalar que modula `MaxWalkSpeed` y `JumpZVelocity` (`AstraeonSuitComponent.cpp:142-158`) y se imprime en el HUD. Cero llamadas a dirección de gravedad |
| Motor | UE 5.7.4 local. `SetGravityDirection` (`CharacterMovementComponent.h:1675`), `ProjectToGravityFloor` (:1698), `GetGravitySpaceZ` (:1701), `HasCustomGravity`, `GetGravityTransform` (`Character.h:528`) — **verificados en la instalación, no supuestos** |
| Malla procedural | `ProceduralMeshComponent` ya en `Build.cs`; `AAstraeonTerrainSurfacePrototype::BuildMeshData` ya emite vértices, triángulos, normales y UV |

**Lectura.** La Fase 1 no reemplaza un sistema de gravedad: lo crea donde no había ninguno. Eso la
abarata. Lo que no regala el motor es la orientación: no hay flag de auto-alineación de cápsula,
malla ni cámara. Ese es el trabajo real y el riesgo real de la Fase 1.

---

## 2. Matriz de migración

### 2.1 Se conserva sin tocar — ~43 de 58 pruebas

Ningún sistema de esta lista consulta geometría del mundo:

Crafting, inventario, bitácora, escáner, ciencia ambiental, traje y protección, hambre y oxígeno,
narrativa y objetivos, comportamiento de criaturas, arranque de sesión, forma y migración del
save, derivación de seeds (`DeriveSeed` con clave versionada y CRC32), `GeneratorVersion`.

Esto es la evidencia del §1.4 del documento rector: **no se reinicia el proyecto**.

### 2.2 Se adapta — cambio de dominio, no reescritura

| Sistema | Archivo | Qué cambia |
|---|---|---|
| Consulta de altura | `AstraeonTerrainField.h/.cpp` → `FAstraeonPlanetSurface` | **Hecho el 2026-09-10.** No fue sólo cambiar la firma: el relieve radial nació como una sola capa de ±180 cm y hubo que portarle las **dos capas** del dominio plano —suelo caminable con límite de escalón, y montañas exentas— además del claro por radio angular. Las constantes son las mismas: son de escala humana |
| Contexto de superficie | `FAstraeonTerrainSurfaceContext` | Todos sus campos son `FVector2D`: centro, origen de Ítaca, claros, exclusiones de montaña. Pasan a dirección planetaria |
| Validador de tránsito | `AstraeonTerrainTraversal.h/.cpp` | BFS sobre malla muestreada con escalón máximo → BFS sobre vértices de patch. La lógica se conserva. `Terrain.TraversalDetectsWalls` —la prueba negativa que exige que el validador falle cuando debe— sobrevive intacta |
| Constructor de malla | `AstraeonTerrainSurfacePrototype.cpp` | `BuildMeshData` pasa a ser el constructor de patch detrás de `IPlanetPatchMeshBackend` |
| Ubicaciones de contenido | `AstraeonRegionTypes.h` | `FAstraeonResourceNode::LocationMeters` y `FAstraeonPointOfInterest::LocationMeters` (`FVector2D`) → dirección + altitud |
| Materialización | `AstraeonRegionMaterializer`, `AstraeonGameInstance.cpp` | `MaterializeCurrentRegion()` rehace la región entera. Incompatible con streaming por patch |
| Revelado de mapa | `AstraeonMapRevealLibrary` | Rejilla 2D → celdas sobre la esfera |
| Vuelo | `AstraeonShipPawn` | `TraceGroundZ` y el techo de 240 m asumen Z global |

### 2.3 Se retira

- El campo de baldosas por `InstancedStaticMeshComponent` de `AAstraeonTerrainField`.
- La plataforma plana de Ítaca (`ItacaPadHeightCm`) tal como está resuelta hoy.
- El mapa `L_AstraeonBootstrap` como mapa de producción.
- La esfera escalada actual **no se retira**: se aísla como `BP_PlanetGravityHarness` (banco de
  pruebas), tal como pide el §1.2 del documento rector.

### 2.4 Contradicciones documentales que bloquean

| Documento | Línea / sección | Problema |
|---|---|---|
| `AGENTS.md` | :82 | Prohíbe literalmente *"Planetas esféricos totalmente transitables"*. Cualquier sesión que empiece leyendo los rectores encuentra la instrucción contraria a lo que va a construir |
| `AGENTS.md` | §5 Límites del MVP | El alcance prohibido debe pasar a ser *contenido masivo planetario*, no *planetas esféricos* |
| `MVP_LA_PRIMERA_SENAL.md` | — | La región puede existir sin planeta completo |
| `GAME_DESIGN_MASTER.md` | — | Sistema estelar en fase posterior |
| `ASTRAEON_PLAN_DESARROLLO_MAPA.md` | — | Región plana primero, planeta después |
| `GUIA_ARTE_PLANETAS_BLENDER.md` | — | Radios de 150 m a 2 km como tiers objetivo; pasan a ser laboratorio |
| `ESTADO_Y_RUTA_MAPA.md` | §6 Bloque C | Sectores planos dentro de Region A; superado por patches |

---

## 3. Las dos consecuencias del corte inmediato

### 3.1 Cuarentena de pruebas

Con el mapa plano retirado, unas 15 pruebas pierden el mundo sobre el que corren. **No se
borran ni se comentan**: se marcan con una razón nombrada y se listan aquí. La lista es el
inventario de la deuda abierta por el corte.

| Prueba | Vuelve en |
|---|---|
| `Astraeon.WorldGen.Terrain.Relief` | Fase 1 — **recuperada el 2026-09-10** |
| `Astraeon.WorldGen.Terrain.SurfaceContract` | Fase 3, movida por [ADR 0005](ADR/0005-cuarentena-surfacecontract-a-fase-3.md) — **recuperada el 2026-09-10** |
| `Astraeon.WorldGen.Terrain.Connectivity` | Fase 2 — **recuperada el 2026-09-10** |
| `Astraeon.WorldGen.Terrain.TraversalDetectsWalls` | Fase 2 — **recuperada el 2026-09-10** |
| `Astraeon.WorldGen.Itaca.DeckRestsOnTerrain` | Fase 3 |
| `Astraeon.WorldGen.Itaca.MaterializerSpecs` | Fase 3 |
| `Astraeon.WorldGen.Region.MaterializerSpecs` | Fase 3 |
| `Astraeon.WorldGen.Region.MarkerApplySpec` | Fase 3 |
| `Astraeon.WorldGen.Region.MarkerVisualIdentity` | Fase 3 |
| `Astraeon.Exploration.Map.RevealCell` | Fase 3 |
| `Astraeon.Exploration.Map.RadiusAndPersistence` | Fase 3 |
| `Astraeon.Art.Region.PresentationPreservesGameplay` | Fase 3 |
| `Astraeon.Art.Itaca.ExteriorAndStations` | Fase 3 |
| `Astraeon.Narrative.Itaca.SurfaceDeploymentLogbook` | Fase 3 |
| `Astraeon.Functional.CriticalPath.FullFlow` | Fase 3 |

**La puerta de la Fase 3 es que esta tabla quede vacía.** Una prueba en cuarentena que nadie
reactiva es una prueba borrada con otro nombre.

### 3.2 Build recuperable

Antes del primer commit que retire el mapa plano:

- Etiqueta git `pre-transicion-plana` sobre `503e3a1`.
- `Builds/WindowsProtagonista/Astraeon.exe` se conserva y se documenta como la última build
  jugable del mundo plano.

Es lo que ya exige la puerta de la Fase 0 del documento rector. Con corte inmediato deja de ser
una formalidad: durante las Fases 1 y 2 es la única build que se puede mostrar o probar a mano.

---

## 4. El trabajo, en orden

### Fase 0 — Transición controlada (documentos y spike en paralelo)

**Rama documental**

- `Docs/ADR/0004-planetas-esfericos-fundacionales.md`.
- Corregir `AGENTS.md` §5 y la línea 82.
- Sincronizar `MVP_LA_PRIMERA_SENAL.md`, `GAME_DESIGN_MASTER.md`,
  `ASTRAEON_PLAN_DESARROLLO_MAPA.md`.
- Reclasificar los radios de `GUIA_ARTE_PLANETAS_BLENDER.md` como Lab.
- Crear `Docs/PHASE_STATUS.md`.
- Marcar el Bloque C de `ESTADO_Y_RUTA_MAPA.md` como superado y anotar que el Bloque B queda
  absorbido por la Fase 2 (§4, Fase 2).

**Rama técnica**

- Etiqueta y build recuperable (§3.2).
- Módulo `Source/Astraeon/Public/Planet/` y `Private/Planet/` con la estructura del §5 del
  documento rector.
- Renombrar la esfera escalada a `BP_PlanetGravityHarness`.
- `UAstraeonPlanetGravityComponent` mínimo: `SetGravityDirection` hacia el centro y orientación
  de cápsula. Nada más.
- Mapa `TL_10_RadialGravity`.
- Baseline de rendimiento registrado en `Docs/evidencia/`.

**Calibración Blender** (ya cubierta en parte por el pipeline existente)

- Congelar plantilla y preset `UE57_AST_V1`; registrar versiones de Blender, Higgsfield y add-on.
- `TL_00_AssetCalibration` con cubo de 1 m, mannequin de 1,83 m y ejes.
- Validar un static mesh, un skeletal mesh, una Action y un shape key como morph.

**Puerta de salida**

- No queda documento rector que prohíba la esfera ni que mande Landscape plano.
- El proyecto compila y las ~43 pruebas de §2.1 siguen verdes.
- Existe la etiqueta recuperable y la cuarentena de §3.1 está poblada.
- El personaje se sostiene de pie en el polo opuesto del harness sin que la cámara ruede.

### Fase 1 — Núcleo planetario

- `FAstraeonPlanetDefinition`: radio, masa, gravedad, nivel del mar, seeds, versión de generador.
  **Nace como struct C++**, no como Data Asset. Ver §6.
- `FAstraeonPlanetCoordinates`: `FaceUvToCube`, `CubeToFaceUv`, dirección ↔ cara/UV.
- Migrar `AstraeonTerrainField` al dominio radial (§2.2). La altura se consulta por dirección
  planetaria global, nunca por UV local aislada: es la única forma de que dos caras coincidan
  en su borde.
- `UAstraeonPlanetGravityComponent` completo: salto, caída, movimiento sobre plano tangente,
  transporte suave del frame de cámara.
- `APlanetRuntime` con las seis caras a LOD bajo.
- Tiers de prueba: Lab 10 km, Target 500 km, Stress 2500 km. **El radio es un dato, no un Scale.**
- `TL_11_CubeSphereClosed`.

**Pruebas nuevas**

- `Astraeon.Planet.Topology.FaceEdgesMatch` — dos caras consultan el mismo valor en su borde.
- `Astraeon.Planet.Height.Determinism` — misma seed, mismo relieve, entre ejecuciones.
- `Astraeon.Planet.Gravity.CardinalPoints` — polos, ecuador y bordes de cara.

**Puerta de salida:** vuelta lógica completa sin perder orientación; sin costura abierta; saltar
y caer funciona en polos, ecuador y bordes; la cámara no da tirones; `Terrain.Relief` sale de
cuarentena.

> Modificado el 2026-09-10 por [ADR 0005](ADR/0005-cuarentena-surfacecontract-a-fase-3.md):
> `Terrain.SurfaceContract` vuelve en la Fase 3. La premisa del §2.2 —que migrar el terreno era
> cambiar la firma y el dominio del ruido— resultó cierta para `Relief` y falsa para
> `SurfaceContract`, que valida sobre todo contenido de región. Se mueve la fecha, no el listón.

### Fase 2 — Patches, LOD, precisión y estado mutable

Esta fase absorbe el **Bloque B** de `ESTADO_Y_RUTA_MAPA.md`, y no por prolijidad: la puerta de
la Fase 2 exige *"ir y volver regenera el mismo patch"*, y hoy `BACKLOG` MV4 registra que **las
criaturas muertas reaparecen al rematerializar**. Con patches que cargan y descargan, ese agujero
pasa de molestia a bloqueo.

- `FAstraeonPlanetPatchAddress` y `StableHash64` con canal de generación y versión.
- `PlanetPatchManager`, `PlanetLODManager`, `PlanetStreamingManager`.
- `IPlanetPatchMeshBackend` envolviendo `BuildMeshData`. Prototipo sobre
  `ProceduralMeshComponent`; la decisión de producción se registra por ADR después de medir.
- Generación en workers, commit en game thread, cancelación y `BuildRevision`.
- Skirts. Delta de LOD ≤ 1 entre vecinos. Stitching sólo si se mide que hace falta.
- Colisión activa sólo en el anillo cercano.
- `LocalReferenceFrameManager` y transición entre frames.
- **`UAstraeonRuntimeStateManager`**: deltas persistentes indexados por dirección planetaria.
- **Save v2 → v3**: `BodyId` + dirección superficial + altitud + coordenadas locales reemplazan
  a `PlayerTransform` e `ItacaOriginCm`. La maquinaria de migración ya existe de v1→v2.
- `TL_12_PatchLOD`, `TL_13_CollisionRing`, `TL_14_FrameTransition`.
- Perfil en Unreal Insights.

**Puerta de salida:** el tier Target de 500 km funciona sin que los patches de alta resolución
crezcan linealmente con el radio; el stress test no produce jitter cerca del jugador; sin grietas
en la ruta de prueba; la colisión no desaparece bajo el jugador; ida y vuelta regenera el mismo
patch; sin hitches recurrentes sobre presupuesto; **una criatura abatida sigue abatida tras
descargar y recargar su patch**; `Terrain.Connectivity` y `Terrain.TraversalDetectsWalls` salen
de cuarentena.

### Fase 3 — La primera señal esférica

- `RegionId` → región planetaria: dirección, extensión angular y altitud.
- Proyectar spawn, recursos, señal y criatura sobre la esfera.
- Adaptar escáner, mapa y bitácora a coordenadas planetarias.
- Materializar rutas críticas antes que decoración.
- Validar transitabilidad sobre la esfera con el validador migrado.
- Llegada controlada mientras el vuelo libre no exista.

**Puerta de salida:** **la tabla de §3.1 queda vacía**; las 58 pruebas verdes sobre la esfera;
guardar, cerrar, abrir y continuar funciona; la misma seed conserva terreno y contenido; ninguna
dependencia funcional de superficie plana; presupuesto de rendimiento mantenido; recorrido de
30–45 min cronometrado a mano (el punto que quedó abierto del Bloque A).

Al pasar esta puerta se recupera una build jugable y el **Producto B** del documento rector queda
cumplido.

---

## 5. Lo que no se toca todavía

Biomas plurales, océano y atmósfera definitivos, nave pilotable con máquina de estados,
sistema estelar, ecología, civilizaciones, ciudades y legado. Son las Fases 4 a 9 y ninguna
empieza antes de que la Fase 3 cierre.

El arte se mantiene en Q1. El pulido de animación de `KNOWN_ISSUES.md` —poses de brazo en cruz,
patinaje de pies, capas de porte— entra como tarea acotada dentro de una fase, no como fase
propia. La lección del §5 de `ESTADO_Y_RUTA_MAPA.md` sigue vigente: **no se pule una fase que
todavía no cerró.**

---

## 6. Deudas que este plan difiere a propósito

| Deuda | Por qué se difiere | Cuándo se paga |
|---|---|---|
| Perfiles en Data Assets | Con un planeta y una región, mover perfiles a datos no desbloquea nada, y las estructuras van a cambiar de forma dos veces durante las Fases 1 y 2. Convertir hoy sería convertir tres veces | Fase 4, con el segundo bioma |
| Renombrado de carpetas al esquema del §5 del documento rector | El módulo `Planet/` nace ya con la estructura correcta. Renombrar `WorldGen/`, `Survival/` y `Knowledge/` sería churn sin beneficio | Cuando exista el primer sistema de civilización |
| Segundo bioma | Es el punto once del primer gran hito y sigue abierto, pero pedirlo sobre geometría que va a cambiar es pedirlo dos veces | Fase 4 |

---

## 7. Riesgos propios de este plan

No repite los del §12 del documento rector. Estos nacen de las dos decisiones de §0.

| Riesgo | Señal temprana | Mitigación |
|---|---|---|
| La cuarentena se vuelve permanente | Una fase cierra sin vaciar su parte de la tabla de §3.1 | La tabla es criterio de puerta, no una nota. Una fase no cierra con pruebas suyas en cuarentena |
| Meses sin build jugable erosionan el juicio sobre el juego | Decisiones de diseño discutidas sin poder probarlas | `TL_10` y `TL_11` son jugables desde la Fase 0. La etiqueta `pre-transicion-plana` queda ejecutable para contrastar sensación de movimiento |
| El spike de la Fase 0 se declara exitoso por compilar | "La gravedad radial funciona" sin haber caminado hasta el polo opuesto | La puerta de la Fase 0 exige el polo opuesto con la cámara estable, probado a mano |
| La orientación de cámara consume la Fase 1 entera | Iteración larga sobre roll y tirones sin criterio de "suficiente" | Es el riesgo mayor de la fase: el motor da dirección de gravedad pero no alineación. Si a mitad de la Fase 1 sigue abierta, se acota a "sin roll involuntario y sin salto de orientación en los polos" y el pulido se difiere |
| Migrar el validador de tránsito rompe la garantía del Bloque A | `TraversalDetectsWalls` se adapta hasta que aprueba siempre | Esa prueba **exige que el validador falle**. Si al migrarla hay que relajarla, la migración está mal, no la prueba |

---

## 8. Primer commit

Rama documental y rama técnica de la Fase 0, en un solo cambio, porque el documento rector pide
que la sincronización no deje un estado intermedio contradictorio:

1. `Docs/ADR/0004-planetas-esfericos-fundacionales.md`.
2. `AGENTS.md` §5 y línea 82 corregidas.
3. Etiqueta `pre-transicion-plana` sobre `503e3a1`.
4. `Docs/PHASE_STATUS.md` con la Fase 0 abierta y la cuarentena de §3.1 poblada.
5. Módulo `Planet/` vacío con su estructura de carpetas.
6. `UAstraeonPlanetGravityComponent` mínimo y `TL_10_RadialGravity`.

Evidencia exigida al cerrarlo: diff identificable, comando de build, resultado de las ~43 pruebas
que deben seguir verdes, captura del personaje en pie en el polo opuesto, y el baseline de
rendimiento registrado.
