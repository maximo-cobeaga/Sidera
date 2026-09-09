# Backlog — ASTRAEON

## 2026-09-07 — Protagonista completo solicitado mediante Bridge

- [x] Auditar fuentes humanas previas, pipeline local y herramientas expuestas.
- [x] Prueba Blender aislada de malla/keyframes con fuente previa conservada.
- [x] Reinspeccionar Bridge: servidor y herramientas `bl_*` disponibles; host `blr:false`.
- [x] Abrir Blender con autorización; proceso PID 11784 responde.
- [x] Bridge conectado; inspección y prueba reversible en Blender 5.2.1 LTS aprobadas.
- [x] Crear/guardar proxy de proporciones de 1.83 m y revisar frente/perfil; sin rig.
- [x] Generación autorizada y ejecutada: job Tripo `8a550175`, 10 créditos, GLB importado.
  Saldo Higgsfield agotado (10/10 del plan free).
- [x] Corregir orientación: el importador glTF deja `rotation_mode='QUATERNION'` y el
  `rotation_euler` escrito por la sesión previa nunca se aplicó.
- [x] LOD0 game-ready por decimación calibrada contra el rostro (233.846 tris).
- [x] Rig `SKEL_Astraeon_Player`: 71 huesos con nomenclatura UE, landmarks medidos sobre la malla.
- [x] Skinning propio por distancia a segmento óseo con filtro de lateralidad; 0 vértices sin peso
  (el bone heat weighting de Blender falla en esta malla).
- [x] 45 Actions a 30 FPS con auditoría de contacto de suela y costura de loop.
- [x] LOD1–3 con pesos y UV conservados; export FBX + round-trip verificado.
- [~] Casco `SK_Astraeon_Helmet`: separado, con 4 materiales y socket, **acabado blockout**.
- [ ] Retopología + bake de normales; bajar LOD0 a 60–80 k tris. *Mayor impacto pendiente.*
- [ ] Separar sets de textura Character / Suit / Gear / Helmet (hoy un único atlas).
- [ ] Shape keys faciales, twist bones y weight painting manual de hombros/axilas/ingle.
- [ ] Mochila y computadora de muñeca como props sobre `socket_backpack` / `lowerarm_l`.
- [ ] Ensayo de importación real en Unreal 5.7.4 y smoke; hoy sólo está validado el FBX.

Evidencia y contrato: [MAIN_CHARACTER_PIPELINE.md](../graphics/characters/main_player/docs/MAIN_CHARACTER_PIPELINE.md)
y [animation_contract.json](../graphics/characters/main_player/animations/animation_contract.json).

## Próximo bloque planificado — terreno regional propio, 2026-09-06

Estado: **reorientado por arquitectura de mundo fija**. Presupuesto de compras cero.
Orden, responsables, archivos, método y aceptación en [PLAN_TERRENO_REGIONAL.md](PLAN_TERRENO_REGIONAL.md).

- [x] T00 — Medir línea base y definir contrato de superficie final.
- [x] A01 — Definir `PlanetProfile`/`RegionProfile` y layout fijo de Region A.
- [x] A02 — Integrar perfiles a sesión, materialización y savegame con migración automática.
- [ ] A03 — Definir superficie diseñada de Region A y sus reglas de colisión/rutas.
- [~] T01 — Prototipo de malla continua creado; diferido hasta que consuma superficie diseñada de Region A.
- [ ] T02 — Integrar Ítaca, recursos, criaturas, obras y compatibilidad de guardados.
- [ ] T03 — Corregir desplazamiento y contacto de criaturas contra obstáculos.
- [ ] T04 — Diseñar dos rutas transitables y referencias regionales por seed.
- [ ] T05 — Producir materiales propios y tres siluetas de roca con pipeline reproducible.
- [ ] T06 — Verificar diez seeds, recorridos, aterrizajes, save/load y build Development.

Siguiente tarea al reanudar: A03, definir la superficie diseñada de Region A, sus corredores,
zonas prohibidas y colisión. El prototipo continuo sigue aislado hasta que pueda consumir esos
datos. A02 compiló, pasó 55 Automation Tests y el smoke crítico de editor-game.

Ver [MVP_WORLD_ARCHITECTURE.md](MVP_WORLD_ARCHITECTURE.md),
[WORLD_ARCHITECTURE_AUDIT.md](WORLD_ARCHITECTURE_AUDIT.md) y ADR 0003.
Este bloque fija la próxima prioridad; las secciones históricas inferiores se conservan
como antecedentes y no invalidan los informes de integración posteriores.

## Personaje — primer lote Blender, 2026-09-06

- [x] `SKEL_Humanoid_A`, 57 huesos y anclajes de equipo.
- [x] Cuerpo vestido Q1 de 1,80 m y manos/antebrazos FP sobre el mismo rig.
- [x] Siete animaciones simples: idle, walk, run, jump, land, interact y grip.
- [x] Pesos/deformación, determinismo, controles negativos y nueve roundtrips FBX.
- [x] Importación Unreal transitoria: dos mallas, siete clips, escala y poses verificadas.
- [ ] Integrar presentación y selección de animación al Character; verificar cámara FP.
- [ ] Siguiente diseño: escáner/cortadora/taladro, luego martillo/maza/ración y sus agarres.
- [ ] Pulir anatomía del guante, solapes de protecciones, transiciones e IK de pies.

Ver `HUMANOID_ART.md`. La producción Blender del primer bloque queda hecha en Q1;
su integración jugable y aceptación visual siguen abiertas. No confundir importación
en memoria con contenido persistido ni con animaciones funcionando en la build.

## Arte regional — entrega 2026-09-06

- [x] Siete siluetas originales Q1: fibra, ferrita, recurso de seed, dos vetas, señal y anomalía.
- [x] Generador Blender, controles de escala/determinismo, geometría cerrada y roundtrip FBX.
- [x] Importación Unreal y conexión C++ conservando ids, cantidades, proxies y guardados.
- [x] Test en mundo transitorio con seis seeds, reubicación y fallback para ids futuros.
- [x] Smoke de siete marcadores reales con escáner/recolección por input; feedback del taladro reparado.
- [x] Suite completa 46/46, smoke crítico editor-game y build Development.
- [ ] Revisión humana de lectura/siluetas para ascender a Q2; ver `REGION_ART_INTEGRATION.md`.
- [x] `SKEL_Humanoid_A` + manos FP con prueba de deformación/exportación; ver lote humano arriba.
- [ ] Variantes específicas de recursos de seed, colisión ajustada a silueta, materiales e iluminación.

Actualiza §3.4 de `ASSETS_PENDIENTES_DISENO_ANIMACION.md`; personaje, criatura, estaciones,
nave, animación, UMG, VFX y audio siguen abiertos. Los pendientes antiguos de ESCOTILLA/input
se interpretan junto con las validaciones manuales posteriores ya registradas.

> **2026-09-05 — Cambio de fase.** El propietario validó el recorrido crítico completo a
> mano y decidió cruzar la "Puerta de salida" del MVP (`MVP_LA_PRIMERA_SENAL.md` §9) hacia
> la **fase de mundo vivo**: despegar la nave → volar dentro del planeta → generar planetas
> → viajar a otros planetas, más razas, civilizaciones y crafteo. El §8 de ese documento
> (que congelaba vuelo, planetas y civilizaciones) ya no describe el alcance vigente.
> **Estado, decisiones de arquitectura y trabajo sin verificar: ver
> `Docs/HANDOFF_FASE_MUNDO_VIVO.md`.**

## Pipeline de arte — entrega 2026-09-05

- [x] Auditoría real, taxonomía completa de familias y prioridades MVP/futuro.
- [x] Dirección visual, especificación humana/Ítaca/cuadrúpedo y contrato planetario conservado.
- [x] Pipeline bpy reutilizable con configuración, validación, exportación individual y reportes JSON.
- [x] Kit blockout de escala e Ítaca: cinco assets, preview y fuente local reproducible.
- [x] 15 controles de prueba, 5 validaciones de fuente, 5 roundtrips FBX y 5 importaciones Unreal con escala correcta.
- [x] Manifiesto con dependencias y estado separado de integración jugable.
- [ ] Siguiente arte: ensayo aislado de montaje/colisión de interior, especialmente hueco del marco; preservar mapa bootstrap.
- [ ] Conectar presentación de estancia a ARGOS/escotilla tras diagnóstico de input; smoke manual de AC-03.
- [ ] Recursos/señal, después rig cuadrúpedo y variante; orden detallado en `ART_ASSET_MASTER_PLAN.md`.

Este bloque no sustituye la prioridad del fallo manual de ESCOTILLA ni certifica un nuevo build del juego.

## Estado de hitos

| Hito | Estado | Criterio de salida |
| --- | --- | --- |
| H0 — Bootstrap reproducible | Completo | Proyecto C++ creado, Git/LFS configurados, build del editor/juego, mapa bootstrap y tests base pasan. |
| H1 — Caminata vertical | En progreso avanzado | Primera persona, menú, seed, medición, bitácora y guardado mínimo. Falta validación manual completa. |
| H2 — Mundo y supervivencia | En progreso avanzado | Región procedural, ambiente completo, consecuencias del traje y mapa revelable. Falta balance/UX. |
| H3 — Vida y conocimiento | En progreso avanzado | Mob evitable, escaneo progresivo y bitácora integrada. Falta pulido audiovisual. |
| H4 — Recursos y solución | En progreso avanzado | Tres recursos, inventario, receta, barrera superable y marcadores legibles. Falta UX final. |
| H5 — La primera señal | En progreso avanzado | ARGOS, hatch de despliegue, objetivo final y recorrido completo existen. Falta escena Ítaca visual final y ritmo narrativo. |
| H6 — Candidato MVP | Mecánicamente completo | Tests, critical-path smoke, package Development, smoke manual completo (ESCOTILLA incluida), save/close/open/continue y rendimiento: todo confirmado por el usuario en sesión real (2026-09-05). Falta informe final y la escena visual de Ítaca (H5.2, en pausa). |

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
- **H6.4 — Despliegue seguro desde hatch**: el hatch usa un punto de despliegue centralizado, detiene movimiento al teleportar, existe una plataforma runtime dedicada bajo el destino, y el fix anti-caída quedó **confirmado por el usuario en juego real**.
- **H6.5 — Mira y feedback de interacción legible**: se agregó una mira central al HUD y color distintivo para `Estado:`.
- **H6.6 — Interacción `E` tolerante por proximidad**: si el line trace no impacta un marcador, el personaje busca el mejor `AAstraeonRegionMarker` en 450 cm priorizando cercanía a la mira, con fallback a marcador muy cercano. La ESCOTILLA temporal además tiene hitbox alto. **Estado manual: insuficiente; el usuario confirmó que `E` frente a ESCOTILLA sigue sin hacer nada.** Ver `Docs/HANDOFF_ESCOTILLA_INTERACCION.md`.
- **H6.7 — Sincronización de pruebas con UI actual**: las pruebas de HUD/hints/labels ahora validan las cadenas españolas presentes en el build temporal.
- **H6.8 — ESCOTILLA resuelta y smoke manual completo (2026-09-05, Rebuild10-12)**: causa raíz real era una roca procedural sin exclusión que podía bloquear el acceso físico a la ESCOTILLA (fix: exclusión de 300 cm alrededor de cada punto de interés de Ítaca), agravada porque `DisableAllScreenMessages` enmascaraba cualquier debug en pantalla. El usuario confirmó en sesión real: ESCOTILLA, recolección, crafting, señal, save/close/open/continue (`F6`/`F10`) y rendimiento estable. Ver `Docs/KNOWN_ISSUES.md`.
- **H6.9 — Correr y bitácora legible**: `Shift` para correr (520→900 cm/s); `L` abre/cierra una vista de bitácora con título+resumen por entrada.
- **H6.10 — `F6`/`F10` en vez de `F5`/`F9`**: evita el choque con los atajos de debug del motor (`viewmode shadercomplexity`, captura de pantalla) activos en builds Development.
- **H3.4 / H4.4 — Escáner completo y anomalía con propósito (2026-09-05)**: cierra el hueco de `MVP §3.5` (el escáner exigía identificar ambiente, recursos, mob y señal/estructura; sólo identificaba ambiente y mob). Ahora escanear un recurso, la fuente de señal o la anomalía produce entradas de bitácora distintas con su nivel de certeza. La anomalía menor dejó de ser contenido muerto: escanearla la registra como *Observada* y presionar `E` de cerca la sube a *Medida*, revelando que comparte frecuencia con la señal. Bug corregido de paso: el generador emite `minor_geologic_anomaly` pero el marcador comparaba contra `minor_anomaly`, así que el jugador veía el id crudo en blanco en vez de "ANOMALÍA" en naranja. Cubierto por `Astraeon.Scanning.Marker.UpdatesLogbook` y ampliación de `Astraeon.WorldGen.Region.MarkerVisualIdentity` (36/36 tests).

- **H2.5 — Protección: la medición se vuelve decisión (2026-09-05)**: implementa el paso 7 del recorrido crítico (`MVP §2`, "Elegir protección básica adecuada") y el requisito `MVP §3.4` ("Decisión sobre protección"), que no existían en el código. Tres módulos —respirador, aislante térmico, sellado de presión— excluyentes entre sí: cada uno mitiga su amenaza a un residuo, ninguno vuelve inmune, y una región fría *y* despresurizada obliga a priorizar. El HUD muestra las amenazas activas del ambiente medido y cuál módulo conviene, de modo que gravedad/temperatura/presión dejan de ser números decorativos. Se persiste en el SaveGame. Teclas `1`/`2`/`3` para equipar, `0` para retirar. Cubierto por `Astraeon.Survival.Protection.MitigatesHazards` y `Astraeon.Survival.Protection.Persists` (38/38 tests).

- **MV1 — Ítaca despega y vuela dentro de la región (2026-09-05)**: primer eslabón de la fase de mundo vivo. Consola de `PILOTAJE` dentro de Ítaca, `E` despega; vuelo en tercera persona con `WASD`/ratón/`Espacio`/`Ctrl`/`Shift` y aterrizaje con `E`. Techo atmosférico de 240 m, narrativo: ARGOS no registra destinos fuera de la región. **Cambio de arquitectura**: Ítaca dejó de estar clavada en `(0,0)` — `UAstraeonGameInstance::ItacaOriginCm` (persistido) define su posición, y aterrizar la reubica junto con estancia, consolas y escotilla. Compila, 38/38 tests. **Falta prueba manual del vuelo**: ver `Docs/HANDOFF_FASE_MUNDO_VIVO.md` §5.

- **MV2 — Mesa de fabricación y protección con costo (2026-09-05)**: estación `FABRICACIÓN` dentro de Ítaca (se mueve con la nave). `E` abre la mesa; `0` fabrica el resonador, `1`/`2`/`3` los módulos de protección. Con la mesa cerrada, esas mismas teclas equipan. **Los módulos dejaron de ser gratuitos**: hay que fabricarlos con recursos para poder equiparlos, lo que cierra el bucle medir → entender → fabricar lo correcto → equipar. Regla anti-bloqueo explícita: la mesa **no deja gastar insumos que el resonador todavía necesita** y lo explica en pantalla, así que fabricar accesorios nunca puede romper el recorrido crítico. Bug corregido de paso: el generador declara cuánto rinde cada veta (3 los básicos, 2 el característico) y la recolección lo ignoraba, entregando siempre 1 unidad. Cubierto por `Astraeon.Crafting.Fabricator.Recipes` y `Astraeon.Resources.Nodes.DeclaredYield` (40/40 tests).
- **MV1.1 — Fix de aterrizaje lejos de Ítaca (2026-09-05)**: reportado por el propietario. Dos causas: el destino de la ESCOTILLA seguía siendo absoluto `(0,1200,150)` y no se mudaba con la nave; y durante el vuelo el personaje quedaba oculto **sin colisión pero con Tick activo**, así que caía sin suelo y la red anti-caída lo devolvía al punto previo al despegue. Ahora el destino cuelga del origen de Ítaca y el personaje se congela en vuelo (`PrepareForShipFlight`/`RecoverFromShipFlight`, que además reancla el rescate al punto de aterrizaje).

- **MV3 — Inventario gestionable y fin de la plataforma de despliegue (2026-09-06)**: `I` abre una vista de inventario que separa materiales de equipo fabricado, muestra cantidades y la protección equipada. Antes el HUD sólo decía "Inventario: N ítem(s)", lo que dejaba a ciegas a la mesa de crafteo. Además se **eliminó la plataforma de despliegue**: era andamiaje de cuando la ESCOTILLA teletransportaba lejos, y desde que Ítaca aterriza físicamente sólo aparecía como un bloque del tamaño de la nave junto a ella (reportado por el propietario). La ESCOTILLA ahora deja al jugador justo afuera de la nave.
- **MV4 — Combate letal (2026-09-06)**: decisión del propietario ante la alternativa no letal. `weapon_pulse_cutter` se fabrica en la mesa (tecla `4`) y dispara con **click derecho** (el click izquierdo sigue siendo el escáner: medir y matar son gestos distintos a propósito). Las criaturas tienen vida (`FAstraeonCreatureProfile::MaxHealth`), se vuelven hostiles al ser heridas, y al morir entregan `biomass_sample` más una entrada de bitácora. Tres impactos abaten a un Umbra Grazer, para que esquivar siga siendo una opción válida. Cubierto por `Astraeon.Combat.Weapon.KillsCreature` (41/41 tests).
  - **Limitación conocida**: al remateralizar la región (p. ej. al aterrizar) las criaturas muertas reaparecen, porque el spawn se reconstruye desde el layout de la seed. Falta persistir qué criaturas fueron abatidas.

- **MV5 — Relieve de terreno determinista (2026-09-06)**: `AAstraeonTerrainField` genera colinas por seed con `UInstancedStaticMeshComponent`. La altura es una **función pura** `GetHeightCm(seed, x, y)`, consultable sin que el terreno exista: es la pieza que va a necesitar la generación de planetas para colocar cosas sobre el suelo antes de construirlo. El relieve **sólo sube desde Z=0**, así que nunca hunde nada bajo la placa de suelo, y deja llanos los puntos jugables (Ítaca, vetas, señal, criatura, despliegue) para que las colinas sean paisaje y no un obstáculo que entierre una veta. Cubierto por `Astraeon.WorldGen.Terrain.Relief`.
- **MV5.1 — Fix: el relieve amurallaba el mapa (2026-09-06)**: reportado por el propietario — "ya no hay recursos, ni señal, ni anomalía ni mobs". No estaban enterrados sino **amurallados**: con 26 m de altura máxima y celdas de ruido apenas mayores que el tile, dos tiles vecinos podían diferir decenas de metros, y como el terreno es de bloques ese desnivel es un **escalón vertical** infranqueable. Corregido bajando la altura a 10 m y agrandando mucho las celdas de ruido, de modo que el desnivel se reparta entre decenas de tiles. El invariante quedó **codificado en `Astraeon.WorldGen.Terrain.Relief`**, que recorre 6 seeds × 61 × 61 tiles y falla si el escalón entre vecinos supera lo caminable.
- **MV5.2 — Terreno de dos capas: suelo caminable + montañas (2026-09-06)**: segundo reporte del propietario ("los relieves son muy altos para caminar; deberían haber montañas, pero hay que alisar sus capas"). El escalón entre tiles ya cumplía el límite, pero quedaban **dos cortes a cuchillo** que el test no miraba: los tiles bajos no se instanciaban (escalón de 60 cm contra el suelo desnudo) y los tiles dentro de un claro se descartaban por completo (cada punto jugable en el fondo de un pozo vertical). Rediseñado en dos capas: **suelo** de amplitud contenida que siempre se camina, y **montañas** escasas de hasta 52 m que son barreras deliberadas y no se pretende escalar. Cambio conceptual clave: **los marcadores ya no aplanan el terreno, se apoyan sobre él** consultando la altura — para eso se hizo pura y consultable la función. Sólo Ítaca conserva un llano, con rampa de 70 m calculada para respetar el escalón. El test verifica ahora tres invariantes separados: suelo caminable, montañas que **de verdad se elevan**, y llano de Ítaca sin acantilado.
- **MV5.3 — Tres regresiones del terreno corregidas (2026-09-06)**: reportadas por el propietario. (1) **No se podía salir de la nave, había una pared de roca donde estaba la escotilla**: `MountainScale` comparaba la rampa suave contra cero, y como `SmoothStep` devuelve algo mayor que cero para cualquier distancia no nula, **la exclusión de montañas nunca excluyó nada** y una montaña podía crecer sobre la puerta. Ahora hay una comprobación explícita de radio (`IsWithinAnySpot`), cubierta por test. (2) **Hueco gigante al despegar**: el llano excavaba el suelo hasta Z=0 en un radio de 70 m, o sea un cráter. Ahora el claro es una **meseta a la altura del suelo local**, lo que además permitió bajar el radio a 26 m; Ítaca se posa sobre esa cota tanto al empezar como al aterrizar. (3) **Tirones al cambiar de piloto a estacionar y al iniciar**: el terreno añadía ~14.000 instancias **de a una**, recocinando la colisión en cada llamada; ahora se acumulan y se entregan en lote, y el campo se redujo de 420 a 320 m.
- **MV6 — Herramientas de extracción y vetas profundas (2026-09-06)**: `FAstraeonResourceNode::RequiredToolId` permite vetas que la mano no alcanza. La seed genera dos (`cryo_ferrite_vein`, `resonant_quartz_vein`, rótulo `VETA PROFUNDA` en azul) que exigen el `tool_core_drill`, fabricable en la mesa (tecla `5`). Se ven desde el principio: dan una razón concreta para volver a un sitio ya explorado. **Los tres recursos del recorrido crítico siguen siendo recolectables a mano**, así que la herramienta abre contenido nuevo en vez de bloquear el existente. Cubierto por `Astraeon.Resources.Nodes.ToolGated`.
- **MV4.1 — La mesa se cierra al alejarse (2026-09-06)**: reportado por el propietario — abrías la mesa, te ibas de la nave y el panel quedaba abierto. Ahora se cierra al superar los 5 m desde la estación.

- **Inventario de arte pendiente (2026-09-06)**: `Docs/ASSETS_PENDIENTES_DISENO_ANIMACION.md` lista todo lo que falta **modelar y animar**, levantado desde el código y no desde el plan previo. Dato central: el proyecto tiene **0 esqueletos, 0 animaciones, 0 VFX, 0 audio y 0 UMG**; salvo el kit de blockout de Ítaca, todo lo visible son primitivas del motor escaladas. Incluye especificaciones de escala vigentes, dónde enchufa cada asset en el código y qué no se puede romper (ids de guardado, escalón de 45 cm, hueco de escotilla, Ítaca ya no está en el origen).

- **MV7 — La supervivencia por fin tiene consecuencia (2026-09-06)**: hasta ahora la salud llegaba a 0 y **no pasaba nada**, y el oxígeno **nunca se recargaba**. Eso volvía decorativo todo el sistema de supervivencia: daño ambiental, amenaza de criaturas, módulos de protección y combate letal no tenían desenlace. Ahora: quedarse sin oxígeno **asfixia**; llegar a 0 de salud dispara un **rescate de emergencia de ARGOS** que devuelve al jugador a Ítaca; y estar **dentro de Ítaca recarga** oxígeno y salud, lo que convierte a la nave en refugio y le da un motivo mecánico a la base móvil. El rescate **cuesta la carga opcional** (muestras de biomasa y vetas profundas) pero nunca los insumos del recorrido crítico ni el equipo fabricado — la misma regla que ya aplica la mesa de fabricación: el juego castiga lo opcional y protege el camino. El HUD avisa en rojo antes de llegar. Cubierto por `Astraeon.Survival.Recall.CostsOptionalCargo` y `Astraeon.Survival.Suit.HavenAndSuffocation`.

- **MV8 — Sistema de construcción (2026-09-06)**: cadena completa **tierra → ladrillo → obra**. `E` con el taladro sobre el terreno extrae **regolito** (el taladro gana un segundo uso en vez de inventar otra herramienta); la mesa lo convierte en **ladrillos**. Con el **martillo de obra** (`B`) se entra en modo construcción: fantasma translúcido que sigue la superficie apuntada, **verde si se puede colocar y rojo si falta material** —la respuesta llega antes de gastar, no después—, `Q` cambia entre muro/plataforma/pilar, `R` rota 45°, click izq. coloca y click der. demuele con la **maza de demolición**, que devuelve menos de lo que costó. Las obras **se guardan como datos y se reconstruyen** al materializar la región, porque la región se rehace entera cada vez que Ítaca aterriza. El tick del personaje pasa a cada frame sólo en modo construcción, ya que 0,2 s alcanzan para supervivencia pero no para un fantasma que sigue la mira. Cubierto por `Astraeon.Building.Loop.PlaceDemolishPersist`.
- **MV8.1 — Inventario legible y fix de `Enter` (2026-09-06)**: el inventario pasa a cuatro categorías (materiales, construcción, herramientas, equipo) con nombres en castellano y cantidades alineadas; los ids crudos se traducen sólo para mostrarse, sin tocar las claves de guardado. Bug corregido de paso: **`StartSelectedNewGame` no verificaba `bMenuVisible`**, así que pulsar `Enter` durante la partida reiniciaba la expedición y borraba el progreso en silencio. Lo mismo en `ContinueSavedGame`.

- **MV9 — Objeto en mano, barra rápida y hambre (2026-09-06)**: primer paso hacia el norte declarado por el propietario, *"quiero un juego jugable, no un modo historia"*. **Tener la herramienta guardada ya no basta: hay que llevarla en la mano** (teclas `1`-`6`), lo que convierte el inventario en una decisión en vez de una lista. El HUD muestra la barra rápida y qué se empuña. Las teclas numéricas son **contextuales**: con la mesa abierta fabrican, cerrada eligen ranura; la protección pasó a `P` (cicla entre los módulos fabricados). Nuevo bucle de supervivencia a largo plazo: **el hambre baja siempre —también dentro de Ítaca, que da aire y cura pero no alimenta—**, llegar a cero produce inanición, y las **raciones** se fabrican con biomasa de criaturas cazadas y se comen con `F`. Cierra `cazar → biomasa → raciones → saciedad`. Mano y saciedad se persisten. Cubierto por `Astraeon.Survival.Hand.HotbarAndHunger`.
- **Brief de diseño unificado en `graphics/ASTRAEON_DESIGN_BRIEF.md` (2026-09-06)**: a pedido del propietario, todo lo que hay que diseñar en un solo archivo: medidas canónicas que no se pueden romper, personaje, objetos de mano, **sistema modular de criaturas** (familias de esqueleto, ranuras, sockets con nombre fijo, reglas de escala y presupuestos), **contrato modular de razas**, nave, piezas de construcción, entorno, UI, VFX y audio, con orden de producción sugerido.

## Próximas tareas desbloqueadas

1. **Animación del protagonista — calidad de locomoción (reportado por el propietario, 2026-09-08)**
   - Camina raro, salta raro y los brazos se ven mal. El personaje ya es visible y funcional; esto es pulido de animación, no un fallo.
   - Los 45 clips del cuerpo posan los brazos en cruz (medido: eje Z de brazo al frente usado entre 0,13 y 0,46; manos a x = ±0,48 m). Nunca tuvieron pasada de pulido.
   - En primera persona los brazos están congelados en la pose de agarre por `AN_HandsFP_*`.
   - Sin medir: patinaje de pies contra la velocidad real de `CharacterMovement`, y el salto como clip suelto sin despegue/vuelo/aterrizaje.
   - Detalle en `Docs/KNOWN_ISSUES.md` → "la animación del protagonista se ve rara".

2. **H5.2 restante — Ítaca/narrativa visual (en pausa, a cargo de otra sesión de trabajo)**
   - Piso de Ítaca superpuesto con la nueva geometría de `AAstraeonItacaInterior` (paredes/caja). Ver `Docs/ART_ASSET_MASTER_PLAN.md` → "Hallazgo de smoke manual post-integración".
   - ESCOTILLA como puerta física (backlog, no bloqueante).
   - Mejorar lectura/ritmo del briefing ARGOS.

3. **H6 — Informe final y cierre de MVP**
   - Redactar informe final con limitaciones conocidas (ver `Docs/KNOWN_ISSUES.md` y `Docs/TEST_REPORT.md`).

4. **Pulido futuro no bloqueante**
   - Sonido/partículas/UI de alerta.
   - Arte final y mejor composición visual (depende de H5.2).

## Cerrado 2026-09-08 — Protagonista visible en juego

- El personaje **se ve**: cuerpo con casco, mochila y computadora de muñeca en tercera
  persona (tecla V), y mano con escáner en primera. Verificado con captura dentro de partida
  en editor y repetido sobre el ejecutable empaquetado.
- Causa del "personaje invisible": la importación FBX de animaciones sueltas perdía la
  conversión metros→centímetros del Armature; al evaluar cualquier clip la pose colapsaba a
  1/100. Corregido en la importación y en los 59 clips ya existentes.
- Equipo montado con `SetLeaderPoseComponent`; manos de primera persona dentro del encuadre.
- 55 pruebas automáticas, `BUILD SUCCESSFUL`, smoke de cámara y recorrido crítico sobre el
  ejecutable, todo en verde.
- Detalle: `Docs/INVESTIGACION_PERSONAJE_INVISIBLE.md`.

## Cerrado 2026-09-08 — Protagonista

- Personaje optimizado (65.284 tris, 4 LOD coherentes, 75 huesos), texturas separadas
  Character/Suit/Gear/Helmet, equipo con UV y mochila ajustada, tres morph targets.
- Validado en Unreal 5.7.4 e **integrado** como cuerpo de sombra de `AstraeonPlayerCharacter`.
- Build, 55 pruebas automáticas, smoke en editor y smoke empaquetado en verde.
- Detalle y limitaciones: `Docs/PENDIENTE_PROTAGONISTA.md`.

Queda abierto del personaje, no bloqueante:

- Rediseño de la forma del casco (sombreado y texturas ya resueltos).
- Retopología densa de cabeza si alguna vez hay cámara sobre la cara (habilitaría parpadeo
  y apertura real de boca).
- Seis gestos de herramienta autorizados en Blender y sin exportar.

## Congelado fuera del MVP

- Galaxy procedural con biomas/sistemas múltiples.
- Vuelo libre, múltiples planetas completos, civilizaciones, economía avanzada, colonias o sistemas sociales.
- Cualquier expansión de visión completa hasta cerrar el vertical slice `La primera señal`.
