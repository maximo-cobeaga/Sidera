# Estado de fases — ASTRAEON

## Actualización 2026-09-10 — **Fase 1 CERRADA**. Fase activa: **Fase 2 — Patches, LOD y precisión**

La puerta de la Fase 1 pasó entera, con los cinco criterios confirmados a mano por el propietario
y la lista de cierre de `AGENTS.md` §15.1 completa.

> **Desvío de nomenclatura aceptado por el propietario el 2026-09-10.**
> `PLAN_TRANSICION_EJECUCION.md` §4 pedía un test de Automation llamado
> `Astraeon.Planet.Gravity.CardinalPoints`. No existe con ese nombre: lo que hay es el smoke
> `-AstraeonSmokePlanetCardinals` (`RunPlanetChecks.ps1 -Check Cardinals`), que recorre las **26
> direcciones** —6 centros de cara, 12 aristas y 8 esquinas— sobre un mundo y un personaje vivos.
> Es cobertura equivalente o mayor; lo que no da es el nombre. Se acepta el smoke como evidencia
> del criterio y **no se registra el test faltante como deuda**, porque no la hay: un test de
> Automation no puede levantar el mundo que ese criterio necesita.

---

## Fase activa: **2 — Patches, LOD, precisión y estado mutable**

Abierta el 2026-09-10. Entregables y puerta en `PLAN_TRANSICION_EJECUCION.md` §4. Absorbe el
Bloque B de `ESTADO_Y_RUTA_MAPA.md`.

### Qué está permitido hacer hoy

- `FAstraeonPlanetPatchAddress` y `StableHash64` con canal de generación y versión.
- `PlanetPatchManager`, `PlanetLODManager`, `PlanetStreamingManager`.
- `IPlanetPatchMeshBackend` envolviendo el constructor de malla. La decisión de producción se
  registra por ADR **después de medir**, no antes.
- Generación en workers, commit en game thread, cancelación y `BuildRevision`.
- Skirts. Delta de LOD ≤ 1 entre vecinos. Stitching sólo si se mide que hace falta.
- Colisión activa sólo en el anillo cercano.
- `LocalReferenceFrameManager` y transición entre marcos.
- `UAstraeonRuntimeStateManager`: deltas persistentes indexados por dirección planetaria.
- Save v2 → v3. La maquinaria de migración ya existe de v1→v2.
- `TL_12_PatchLOD`, `TL_13_CollisionRing`, `TL_14_FrameTransition`. Perfil en Unreal Insights.

### Puerta de salida

- [ ] El tier Target de 500 km funciona sin que los patches de alta resolución crezcan
      linealmente con el radio.
- [ ] El stress test no produce jitter cerca del jugador.
- [ ] Sin grietas en la ruta de prueba.
- [ ] La colisión no desaparece bajo el jugador.
- [ ] Ida y vuelta regenera el mismo patch.
- [ ] Sin hitches recurrentes sobre presupuesto.
- [ ] **Una criatura abatida sigue abatida** tras descargar y recargar su patch (`BACKLOG` MV4).
- [ ] `Terrain.Connectivity` y `Terrain.TraversalDetectsWalls` salen de cuarentena.

### Avance técnico 2026-09-10 — P2.1/P2.2 y selección LOD

Direcciones jerárquicas, hash estable con vector fijo, constructor independiente 33×33 con
faldones radiales e índices separados de colisión. La construcción de caras de `TL_11` usa
ahora el patch raíz sin cambiar su relieve. Servicio de workers acotado, cancelación y
revisiones que sobreviven al ciclo descargar/solicitar. El selector quadtree puro ya mantiene
la cobertura completa de las seis caras, delta de LOD ≤ 1, presupuesto máximo de 384 hojas y
selección determinista con predicción de velocidad; **85/85 Automation**, diez nuevas.
Editor y juego Development compilados; cardinales 26/26 y caminata de 250 s en verde,
sin cortes de locomoción. Perfil y revisión de captura registrados en `TEST_REPORT.md`.

Esto verifica contratos de datos, generación y selección LOD. El runtime aún usa seis caras de
resolución fija: conectar el servicio al selector LOD y al backend es la parte de integración de
P2.3. No acredita todavía streaming
visible en Target, colisión por anillos, transición de frame ni persistencia de criaturas.
La cuarentena y los ocho criterios de salida permanecen abiertos.

### Avance técnico 2026-09-10 — P2.3 A/B/C

`TL_11` renderiza por patches con workers, y la colisión coincide triángulo por triángulo con lo
visible. `TL_12_PatchLOD` (50 km) vuela 185 s y hace 918 relevos sin un agujero; el faldón está
medido y cubre delta 1 y 2. Insights encontró un selector cuadrático y se corrigió: p99 del vuelo
de 33,6 a 5,2 ms. **91/91 Automation.** Prueba humana de los dos mapas confirmada (P2.3-D);
**P2.3 cerrado**, sigue P2.4. Esto aporta evidencia a
"sin grietas en la ruta de prueba" y a "sin hitches recurrentes", pero **ninguna casilla de la
puerta se marca todavía**: la puerta pide Target a 500 km, colisión por anillos y estado mutable.

### Deuda que esta fase hereda y debe pagar

El parche de colisión abarca ~37 m pero se rehace cada 3,1 m: doce veces más seguido de lo que su
tamaño exige, recorriendo las 6.144 celdas de las seis caras y recociendo cada vez. El doble búfer
quitó el corte visible, no el desperdicio. Es de esta fase, que es la dueña del anillo de colisión.

### Fuera de alcance en esta fase

Biomas plurales, océano y atmósfera definitivos, nave pilotable, sistema estelar, ecología,
civilizaciones y ciudades. El contenido de región es de la Fase 3.

---

## Histórico: **1 — Núcleo planetario** — puerta PASADA

Cerrada el 2026-09-10. Los cinco criterios confirmados a mano por el propietario sobre
`TL_11_CubeSphereClosed`.

### Criterios de la puerta

| Criterio | Evidencia |
|---|---|
| Vuelta lógica completa sin perder orientación | confirmado a mano por el propietario el 2026-09-10 |
| Sin costura abierta entre caras | confirmado a mano; `Planet.Topology.FaceEdgesMatch` en verde |
| Saltar y caer en polos, ecuador y bordes | confirmado a mano; smoke cardinal en 26 direcciones |
| La cámara no da tirones | confirmado a mano, en primera y tercera persona |
| `Terrain.Relief` fuera de cuarentena | recuperada el 2026-09-10 sobre el contrato radial, con una aserción más |

### Evidencia de cierre (`AGENTS.md` §15.1)

| | |
|---|---|
| Commit o diff identificable | rama `fase-1-nucleo-planetario`, 5 commits |
| Comando de build | recompilación completa del módulo, `Succeeded` |
| Resultado de pruebas | `Automation` 75/75, `Cardinals` PASS, `Walk` 250 s `RESULTADO=OK` |
| Captura | `PlanetWalkLab.png`, `PlanetCardinalLab.png` |
| Perfil de rendimiento | 205,3 FPS medios, p99 6,99 ms, 1 hitch de arranque |
| Warnings nuevos | cero |
| Limitaciones y próximo paso | registradas abajo |
| Ninguna prueba propia en cuarentena | cumplido: `Relief` volvió, `SurfaceContract` pasó a Fase 3 por ADR 0005 |

### Limitaciones conocidas al cerrar

- El parche de colisión abarca ~37 m pero se rehace cada 3,1 m: doce veces más seguido de lo que
  su tamaño exige, recorriendo las 6.144 celdas de las seis caras y recociendo cada vez. Ya no se
  ve —el doble búfer quitó el corte— pero es trabajo desperdiciado. Es de la **Fase 2**, dueña del
  anillo de colisión.
- El p99 pasó de 5,41 ms el 2026-09-09 a 6,99 ms. Dentro del presupuesto de §8 con más de 3× de
  margen: el objetivo del percentil 1 % es 45 FPS y esto son 143.
- El contrato de superficie con contexto de región sigue en `FVector2D`. Es de la **Fase 3**.

### Próximo paso

Fase 2 — patches, LOD, precisión y estado mutable persistente. Abierta arriba.

## Histórico: 2026-09-10 — Fase 1: núcleo técnico verificado, puerta aún abierta

La Fase 0 está cerrada y la fase activa es **Fase 1 — Núcleo planetario**. Ya están completos
el contrato de definición, las coordenadas globales, la altura radial determinista, el runtime
cube-sphere de seis caras y `TL_11_CubeSphereClosed`. La evidencia actual es `75/75` tests de
Automation, smoke cardinal en 26 direcciones y caminata planetaria sostenida de 270 s con
`RESULTADO=OK`. La fase no se declara cerrada todavía: falta reactivar explícitamente las dos
pruebas de terreno que siguen en cuarentena (`Terrain.Relief` y `Terrain.SurfaceContract`) y
registrar una prueba humana breve del salto/cámara sobre este mapa.

### Sesión humana del 2026-09-10 sobre `TL_11_CubeSphereClosed`

El propietario jugó el mapa. **Cuatro de los cinco criterios perceptuales quedaron confirmados a
mano**: orientación tras la vuelta, ausencia de costura, salto y caída, cámara sin tirones, y el
salto en movimiento —que era la confirmación que arrastraba la Fase 0 desde el 2026-09-09—.

El quinto devolvió defecto: **la locomoción se veía trabada y al correr además frenaba**. No era
animación. La colisión cercana se rehacía con un solo componente y dejaba al jugador sin suelo dos
frames cada 312 cm, la velocidad caía a 0 y el selector elegía `Idle`: el ciclo de paso se
reiniciaba 1,3 veces por segundo. Corregido con doble búfer de colisión y medido —de 53 cortes en
40 s a 0, con 24 % más de distancia recorrida—, con guardián permanente en el smoke. Detalle en
`KNOWN_ISSUES`.

**Queda pendiente que el propietario vuelva a mirar la locomoción ya corregida.** Con eso, el
criterio humano de la puerta queda entero.

En la misma sesión reportó tres cosas del protagonista que **no bloquean esta puerta** —falta
quitarse el casco, mano de primera persona distinta a la de tercera y escáner visible sólo en
primera—: son entregables de la **Fase 5** y están en `BACKLOG.md`.

Fase activa, puerta de salida y cuarentena de pruebas. Es el archivo que un agente lee para saber
**qué está permitido hacer hoy**. Se actualiza al abrir y al cerrar cada fase.

Fases definidas en `ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` §6. Orden de trabajo y criterios
detallados en `PLAN_TRANSICION_EJECUCION.md` §4.

---

## Histórico: **0 — Transición controlada** — puerta PASADA

Abierta y cerrada el 2026-09-09. Documentos y spike técnico **en paralelo** (decisión del
propietario). Los seis puntos de la puerta están cumplidos y el criterio de orientación lo
confirmó el propietario jugando.

> **Queda una confirmación pendiente, y no es de la puerta.** El arreglo del salto en movimiento
> (`KNOWN_ISSUES`, 2026-09-09) está medido —racha en el aire de 1,02 s frente a los 116 s del
> defecto— pero **no probado a mano**. Conviene un minuto de partida antes de dar por buena la
> locomoción de la Fase 1, porque es lo último que se tocó de lo que ya funcionaba.
>
> La **Fase 1 puede empezar**: su primer entregable no depende de esa confirmación.

### Puerta de salida

- [x] No queda documento rector que prohíba la esfera ni que mande Landscape plano.
- [x] El proyecto compila y **las 69 pruebas pasan** (58 previas + 9 del marco planetario + 2 del baseline).
- [x] Existe la etiqueta `pre-transicion-plana` y su build recuperable está documentada.
- [x] La cuarentena de §Cuarentena está poblada, con fase de retorno por prueba.
- [x] La calibración Blender → Unreal respeta escala y ejes (`evidencia/CALIBRACION_BLENDER_UNREAL.md`).
- [x] El personaje se sostiene de pie y la cámara no rueda. **Confirmado a mano por el propietario el 2026-09-09**: cámara correcta y vuelta al mundo completa. Medido además con `-AstraeonSmokePlanetWalk`: 0,0% de frames desalineados en una vuelta entera.

### Rama documental

- [x] `Docs/ADR/0004-planetas-esfericos-fundacionales.md`.
- [x] `AGENTS.md` sincronizado: misión, fuentes de verdad, §4.1 restricciones planetarias,
      §5 límites (la prohibición de la esfera transitable era el bloqueo), §7 cuarentena,
      §8 presupuesto, §9 seeds en workers, §11 identidad planetaria, §15 cierre por fase.
- [x] `Docs/PHASE_STATUS.md` (este archivo).
- [x] `Docs/MVP_LA_PRIMERA_SENAL.md`: la región se materializa sobre el planeta esférico.
- [x] `Docs/GAME_DESIGN_MASTER.md`: coordenadas, esfera y gravedad se adelantan.
- [x] `Docs/ASTRAEON_PLAN_DESARROLLO_MAPA.md`: cube-sphere y patches pasan a las primeras fases.
- [x] `Docs/GUIA_ARTE_PLANETAS_BLENDER.md`: radios de 150 m a 2 km reclasificados como Lab.
- [x] `Docs/ESTADO_Y_RUTA_MAPA.md`: Bloque C superado, Bloque B absorbido por la Fase 2.

### Rama técnica

- [x] Etiqueta `pre-transicion-plana` sobre `503e3a1` y build recuperable documentada.
- [x] Módulo `Planet/Coordinates/` y `Planet/Gravity/` creados con su estructura.
- [x] Harness aislado como `AAstraeonPlanetGravityHarness` (C++, no BP: no existía esfera previa que renombrar y `AGENTS.md` §4 reserva los BP para presentación).
- [x] `UAstraeonPlanetGravityComponent` mínimo: `SetGravityDirection` + orientación de cápsula.
- [x] Mapa `TL_10_RadialGravity` (radio 200 m, 4 balizas, generado por script).
- [x] Baseline de rendimiento en `Docs/evidencia/BASELINE_RENDIMIENTO.md` (dos mapas, JSON reproducible).

### Calibración Blender

- [x] Preset `UE57_AST_V1` congelado en `Tools/Blender/presets/`, con comprobación automática de deriva. Versiones de Blender y Unreal registradas. *(Higgsfield: sin generación en esta fase; su versión se registra cuando vuelva a usarse.)*
- [x] `TL_00_AssetCalibration`: cubo de 1 m, mannequin de 1,83 m y tres ejes de largos distintos.
- [x] Static mesh, skeletal mesh, una Action y un shape key como morph: los cuatro validados con números.

### Fuera de alcance en esta fase

Biomas finales, nave pilotable, ciudades y generación masiva de assets.

---

## Cuarentena de pruebas

Abierta por el corte inmediato del mundo plano (ADR 0004). Reglas en `AGENTS.md` §7: no se borra
ninguna, ninguna se relaja, y **una fase no cierra con pruebas suyas todavía aquí**.

Estado: **la Fase 1 ya no tiene deuda aquí**. `Terrain.Relief` volvió el 2026-09-10 migrada al
contrato radial sin perder ninguna aserción, y `Terrain.SurfaceContract` pasó a la Fase 3 por
[ADR 0005](ADR/0005-cuarentena-surfacecontract-a-fase-3.md): valida mayoritariamente contenido de
región —plataforma de Ítaca, claros, exclusiones, Region A— que la Fase 3 va a definir. El resto
sigue verde sobre el mapa plano y la tabla es el inventario de lo que va a entrar.

| Prueba | Vuelve en | Estado |
|---|---|---|
| `Astraeon.WorldGen.Terrain.Relief` | Fase 1 | **RECUPERADA** el 2026-09-10 |
| `Astraeon.WorldGen.Terrain.SurfaceContract` | Fase 3 | movida por [ADR 0005](ADR/0005-cuarentena-surfacecontract-a-fase-3.md) |
| `Astraeon.WorldGen.Terrain.Connectivity` | Fase 2 | **RECUPERADA** el 2026-09-10, sobre la esfera |
| `Astraeon.WorldGen.Terrain.TraversalDetectsWalls` | Fase 2 | **RECUPERADA** el 2026-09-10, sobre la esfera |
| `Astraeon.WorldGen.Itaca.DeckRestsOnTerrain` | Fase 3 | verde |
| `Astraeon.WorldGen.Itaca.MaterializerSpecs` | Fase 3 | verde |
| `Astraeon.WorldGen.Region.MaterializerSpecs` | Fase 3 | verde |
| `Astraeon.WorldGen.Region.MarkerApplySpec` | Fase 3 | verde |
| `Astraeon.WorldGen.Region.MarkerVisualIdentity` | Fase 3 | verde |
| `Astraeon.Exploration.Map.RevealCell` | Fase 3 | verde |
| `Astraeon.Exploration.Map.RadiusAndPersistence` | Fase 3 | verde |
| `Astraeon.Art.Region.PresentationPreservesGameplay` | Fase 3 | verde |
| `Astraeon.Art.Itaca.ExteriorAndStations` | Fase 3 | verde |
| `Astraeon.Narrative.Itaca.SurfaceDeploymentLogbook` | Fase 3 | verde |
| `Astraeon.Functional.CriticalPath.FullFlow` | Fase 3 | verde |

`Astraeon.WorldGen.Terrain.TraversalDetectsWalls` merece vigilancia propia: existe para **exigir
que el validador falle** cuando la región no se puede recorrer. Si al migrarla a patches hay que
debilitar lo que pide para que apruebe, el defecto está en la migración.

---

## Fases siguientes

Ninguna empieza antes de que pase la puerta de la anterior.

| Fase | Nombre | Puerta principal | Estado |
|---:|---|---|---|
| 0 | Transición controlada | ADR y documentos sincronizados; spike de gravedad radial en pie | **PASADA** |
| 1 | Núcleo planetario | Gravedad y cámara estables sobre una esfera cerrada | **PASADA** |
| 2 | Patches, LOD y precisión | Streaming sin grietas ni hitches; estado mutable persistente | **en curso** |
| 3 | La primera señal esférica | Vertical slice completo; **cuarentena vacía** | pendiente |
| 4 | Planeta visual y biomas | Seis biomas y rendimiento aprobado | pendiente |
| 5 | Protagonista, Ítaca y vuelo | Ida y vuelta sin carga perceptible | pendiente |
| 6 | Sistema estelar | Persistencia entre cuerpos | pendiente |
| 7 | Mundo vivo | Ecología, clima y progresión estables | pendiente |
| 8 | Primer contacto | Ciudad coherente, transitable y narrativa | pendiente |
| 9 | Sociedad y legado | Consecuencias persistentes | pendiente |
| 10 | Expansión y lanzamiento | Build estable, medida y testeada | pendiente |

Al pasar la Fase 2 queda cumplido el **Producto A** (núcleo planetario); al pasar la Fase 3, el
**Producto B** (vertical slice esférico); la Fase 10 cierra el **Producto C** (juego objetivo v1).
