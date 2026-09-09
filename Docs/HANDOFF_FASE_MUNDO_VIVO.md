# Handoff — Fase "mundo vivo": Ítaca como nave

Fecha: 2026-09-05. Este documento captura la decisión de salir del MVP y el estado exacto
del primer eslabón (despegue y vuelo), incluido lo que **todavía no está verificado**.

## 1. Decisión: se cruzó la puerta de salida del MVP

El propietario validó a mano el recorrido crítico completo (ESCOTILLA, recolección,
crafting, señal, save/close/open/continue, rendimiento estable). Eso cumple la
"Puerta de salida" de `MVP_LA_PRIMERA_SENAL.md` §9, que ofrece cinco caminos. Se eligió
**iniciar la fase de mundo vivo**.

Consecuencia formal: varias cosas que `MVP_LA_PRIMERA_SENAL.md` §8 congela
explícitamente (vuelo libre, planetas completos, civilizaciones) dejan de estar
congeladas. Ese §8 ya no describe el alcance vigente del proyecto.

## 2. Visión declarada por el propietario, en su orden

1. **Despegar la nave.**
2. **Viajar en la nave dentro del planeta**, sin poder salir del planeta todavía, porque
   no existen otros destinos.
3. **Generar planetas.**
4. **Viajar por el espacio en Ítaca a otros planetas.**

Además, fuera de esa cadena: razas, civilizaciones, mesa de crafteo, más crafteos, "y bueno
más".

### Nota de dependencias

El orden es sensato, pero el paso 2 depende parcialmente del 3 para tener sentido jugable:
la superficie actual es un cubo plano de **1200 × 1200 m**, que a velocidad de nave se
cruza en segundos. Volar se vuelve interesante recién cuando el mundo crece. La
implementación actual del paso 2 sirve para **probar la mecánica**, no para entregar la
experiencia final de vuelo.

## 3. Decisión de arquitectura: Ítaca es la nave, y se mueve

La ficción dice que Ítaca es la nave y el jugador vive adentro. Por eso despegar no es
subirse a un vehículo aparte: **es que la estancia entera se levante**.

Eso obligó a romper un supuesto que estaba clavado en todo el código: que Ítaca vive en el
origen del mundo `(0,0)`.

- `UAstraeonGameInstance::ItacaOriginCm` (nuevo, **persistido en el SaveGame**) es ahora la
  posición de Ítaca.
- `UAstraeonRegionMaterializer::BuildItacaActorSpecs(const FVector& OriginCm)` recibe ese
  origen y coloca consola ARGOS, consola de pilotaje y escotilla **relativas a él**.
- `AAstraeonGameModeBase::MaterializeCurrentRegion()` mueve el actor
  `AAstraeonItacaInterior` a ese origen y remateraliza los marcadores alrededor.
- La exclusión de rocas evalúa `AAstraeonItacaInterior::IsInsideFootprint()` en **espacio
  local de Ítaca** (`RockXY - ItacaOriginXY`), para no tener que cambiar esa función, que
  pertenece al trabajo de arte.

**Aterrizar cambia el origen de Ítaca.** Volar deja de ser turismo: es mudar la base.

### Transición interior/exterior

No se intentó mover el interior físicamente con el jugador adentro (problema clásico y
caro). En su lugar:

- Al despegar: el personaje se oculta y se desactiva su colisión, se oculta el interior y
  sus consolas, y el control pasa a `AAstraeonShipPawn` con cámara en tercera persona.
- Al aterrizar: se destruye la nave, se remateraliza la región con el nuevo origen, se
  vuelve a poseer el personaje y se lo coloca dentro de la estancia ya reubicada.

## 4. Estado del código (paso 1 y 2)

### Archivos nuevos

- `Source/Astraeon/Public/Ship/AstraeonShipPawn.h`
- `Source/Astraeon/Private/Ship/AstraeonShipPawn.cpp`

### Archivos modificados

- `AstraeonGameInstance.h/.cpp`: `ItacaOriginCm` + `SetItacaOriginCm`, persistencia y reset.
- `Persistence/AstraeonSaveGame.h`: campo `ItacaOriginCm`.
- `AstraeonPlayerController.h/.cpp`: `BeginShipFlight()`, `RequestShipLanding()`,
  `IsFlyingShip()`, `GetShipPawn()`; guarda el pawn de a pie en `GroundedPawn`.
- `AstraeonGameModeBase.h/.cpp`: `SetItacaInteriorHidden()`, origen de Ítaca en la
  materialización y en la exclusión de rocas.
- `WorldGen/AstraeonRegionMaterializer.h/.cpp`: origen + consola de pilotaje.
- `WorldGen/AstraeonRegionMarker.cpp`: etiqueta `PILOTAJE` y color para
  `itaca_pilot_console`.
- `AstraeonPlayerCharacter.cpp`: `E` sobre `itaca_pilot_console` despega.
- `AstraeonHUD.h/.cpp`: `BuildFlightLines()` (altitud, velocidad, techo, ayuda de
  aterrizaje).
- `Config/DefaultInput.ini`: eje `ShipLift` (`Espacio` sube, `LeftControl` baja).
- `Tests/AstraeonCoreAutomationTests.cpp`: `Astraeon.WorldGen.Itaca.MaterializerSpecs`
  actualizado a 3 estaciones + cobertura de reubicación rígida.

### Controles de vuelo

`WASD` desplaza · ratón orienta · `Espacio` sube · `Ctrl` baja · `Shift` acelera ·
`E` aterriza.

### Reglas de vuelo

- **Techo atmosférico: 240 m** sobre el suelo. Es narrativo: ARGOS no registra ningún
  destino fuera de la región. Se avisa en el HUD al acercarse.
- Aterrizar exige altitud ≤ 9 m y velocidad ≤ 9 m/s; si no, el descenso se rechaza con
  mensaje.
- Suelo blando: la nave se frena antes de atravesar el terreno en vez de chocar.

## 5. ESTADO DE VERIFICACIÓN

- ✅ **Compila** (`AstraeonEditor Win64 Development`). Hubo un único error, ya corregido:
  el parámetro `bHidden` de `SetItacaInteriorHidden` chocaba con el miembro `bHidden` de
  `AActor` (UHT prohíbe el shadowing); se renombró a `bInteriorHidden`.
- ✅ **Tests: 38/38 en verde**, incluida la cobertura nueva de reubicación rígida de Ítaca.
- ⚠️ **Falta la prueba a mano del vuelo.** Ningún test automático despega, vuela ni
  aterriza: la suite sólo cubre que las estaciones se reubican con el origen. El
  comportamiento de vuelo, la cámara, el techo y la reubicación real de la estancia al
  aterrizar **no han sido vistos por un humano todavía**.

Puntos concretos a mirar en esa prueba manual:

1. Que al despegar no se vea la estancia ni sus consolas flotando abajo.
2. Que al aterrizar el jugador aparezca **dentro** de la estancia reubicada y no bajo tierra
   ni dentro de una pared.
3. Que la escotilla siga funcionando después de mudar Ítaca (usa el nuevo origen).
### Vuelo probado a mano (2026-09-05) — un bug encontrado y corregido

El propietario voló y aterrizó correctamente, y reportó que **al aterrizar a veces aparecía
en un punto lejano**. Su hipótesis fue exacta: al posar la nave quedaba mirando la
ESCOTILLA y el siguiente `E` lo desplegaba a la superficie.

Dos causas, ambas corregidas:

1. **El destino de la ESCOTILLA no colgaba de la nave.**
   `GetSurfaceDeploymentLocationCm()` devolvía el absoluto `(0, 1200, 150)`, así que
   desplegar después de aterrizar lejos teletransportaba de vuelta cerca del origen del
   mundo. Ahora recibe el origen de Ítaca y el pad se materializa junto a la nave. Cubierto
   por el test `Astraeon.WorldGen.Itaca.MaterializerSpecs`.
2. **Se aterrizaba encarando la ESCOTILLA**, a 400 cm y en el centro de la mira, así que un
   `E` de más disparaba el despliegue sin querer. Ahora se aterriza mirando hacia el
   interior de la estancia (`SetControlRotation(0,180,0)`).

## 6. Trabajo ya cerrado y verificado en esta sesión (sí probado)

- **Escáner completo + anomalía con propósito**: el escáner identifica las cuatro
  categorías que pedía `MVP §3.5`; la anomalía dejó de ser contenido muerto. Bug corregido:
  el generador emite `minor_geologic_anomaly` y el marcador comparaba contra
  `minor_anomaly`, así que se veía el id crudo en blanco. **36/36 tests, probado a mano.**
- **Protección: la medición se vuelve decisión**: implementa el paso 7 del recorrido
  crítico, que no existía en el código. Tres módulos excluyentes (`1` respirador, `2`
  aislante térmico, `3` sellado de presión, `0` ninguno), cada uno mitiga su amenaza a un
  residuo y ninguno vuelve inmune. HUD muestra amenazas activas y cuál módulo conviene.
  Persistido en el SaveGame. **38/38 tests, probado a mano** (el propietario confirmó el
  cambio de consumo de O2 con y sin respirador).

## 6.b Segunda tanda de la fase mundo vivo (2026-09-05)

**Fix de aterrizaje** (reportado: "aparezco lejos de Ítaca al aterrizar inclinado"). Dos
causas encadenadas, ambas corregidas:

- El destino de la ESCOTILLA era absoluto y no se mudaba con la nave (ver §5).
- Durante el vuelo el personaje quedaba oculto y **sin colisión pero con `Tick` activo**:
  al no detectar suelo caía indefinidamente y `RescueFromVoidIfNeeded` lo devolvía a su
  último suelo seguro, que era **el punto anterior al despegue**. Ahora
  `PrepareForShipFlight()` lo congela (tick y movimiento desactivados) y
  `RecoverFromShipFlight()` lo reubica reanclando el rescate al punto de aterrizaje.

**Mesa de fabricación** (`itaca_fabricator`, estación `FABRICACIÓN` dentro de Ítaca, se
mueve con la nave):

- `E` abre/cierra. Con la mesa abierta, `0` fabrica el resonador y `1`/`2`/`3` los módulos
  de protección; con la mesa cerrada esas mismas teclas equipan. Es deliberado que
  fabricar y equipar compartan teclas.
- **Los módulos de protección dejaron de ser gratuitos**: hay que fabricarlos para poder
  equiparlos. Eso cierra el bucle medir → entender la amenaza → fabricar el módulo
  correcto → equiparlo.
- **Regla anti-bloqueo**: la mesa no permite gastar insumos que el `signal_resonator`
  todavía necesita, y lo dice en pantalla (`reservado para el resonador`). Fabricar
  accesorios nunca puede romper el recorrido crítico.
- Recetas: resonador (1 silicato + 1 ferrita + 1 característico), respirador (1 silicato),
  aislante térmico (1 ferrita), sellado de presión (1 silicato + 1 ferrita).

**Bug corregido de paso**: `FAstraeonResourceNode::Quantity` (3 para los básicos, 2 para el
característico de la seed) **se ignoraba**: recolectar entregaba siempre 1 unidad. Ahora se
honra, lo que además da presupuesto para fabricar.

Estado: compila, **40/40 tests**. Falta prueba manual de la mesa.

### Respuesta a "¿las elevaciones detrás de Ítaca son un efecto visual?"

No: es geometría real y sólida. Es la **plataforma de despliegue** (8 × 8 m, 50 cm de alto),
que desde el fix de §5 se remateraliza junto a la nave en cada aterrizaje. Es el sitio donde
deja la ESCOTILLA y se puede caminar encima. Las otras elevaciones son las 12 rocas
procedurales, que **siguen ancladas al origen del mundo** y no se mudan con Ítaca — algo a
revisar cuando se ataque la generación de planetas.

## 6.c Tercera tanda (2026-09-06): inventario y combate letal

- **Plataforma de despliegue eliminada.** El propietario reportó "una elevación del tamaño
  de la nave detrás de donde aterrizo". Era el `Runtime_SurfaceDeploymentPad`, andamiaje de
  cuando la ESCOTILLA teletransportaba a un punto lejano y había que garantizar suelo firme
  allí. Desde que Ítaca aterriza físicamente sobre el terreno, sobra. La ESCOTILLA ahora
  deja al jugador **justo afuera de la nave** (`origen + (900,0,150)`, pasando la puerta en
  X=620) en vez de a 12 m.
- **Inventario gestionable (`I`)**: separa materiales de equipo fabricado, con cantidades y
  la protección equipada. Sin esto la mesa de crafteo operaba a ciegas.
- **Combate letal** (dirección elegida por el propietario frente a la alternativa no letal):
  `weapon_pulse_cutter` se fabrica en la mesa (tecla `4`) y dispara con **click derecho**;
  el click izquierdo sigue siendo el escáner, para que medir y matar sean gestos distintos.
  Criaturas con vida, hostilidad al ser heridas, y `biomass_sample` + entrada de bitácora al
  morir. Tres impactos abaten a un Umbra Grazer.

**Limitación conocida del combate**: `MaterializeCurrentRegion()` reconstruye las criaturas
desde el layout de la seed, así que **las criaturas muertas reaparecen** al remateralizar
(por ejemplo al aterrizar). Falta persistir las bajas en el SaveGame.

## 6.d Cuarta tanda (2026-09-06): relieve, herramientas y vetas profundas

- **Mesa que se cierra al alejarse** (reportado): superar 5 m desde la estación la cierra.
- **Relieve de terreno** (`AAstraeonTerrainField`): colinas deterministas por seed con
  instanced static meshes. Lo importante no es el relieve en sí sino la forma:
  `GetHeightCm(seed, x, y)` es una **función pura consultable sin que el terreno exista**,
  que es exactamente lo que la generación de planetas va a necesitar para posar cosas sobre
  el suelo antes de construirlo. El relieve **sólo sube desde Z=0**, de modo que la placa de
  suelo sigue siendo el piso garantizado y nada queda enterrado; los puntos jugables se
  dejan llanos (claros) para que las colinas sean paisaje y no obstáculo.
- **Herramientas y recursos que las exigen**: `FAstraeonResourceNode::RequiredToolId`. Dos
  vetas profundas por seed exigen `tool_core_drill` (mesa, tecla `5`) y se rotulan `VETA
  PROFUNDA` en azul. Se ven desde el principio para dar una razón de volver a un sitio ya
  explorado. Los tres recursos del recorrido crítico siguen siendo de mano.

Dos tests viejos fallaron por asumir "exactamente 3 recursos"; se reescribieron para
afirmar la intención (los tres primeros son el recorrido crítico y no piden herramienta)
en vez de un número mágico. **43/43 tests.**

### Corrección: el relieve amuralló el mapa (2026-09-06)

Primera versión del terreno: `MaxHeightCm = 2600` con tiles de 1200 cm y celdas de ruido
de 14000/4200 cm. El propietario reportó que **desaparecieron recursos, señal, anomalía y
criaturas**.

No estaban enterrados: estaban **amurallados**. El terreno es de bloques, así que el
desnivel entre dos tiles vecinos es un **escalón vertical**, no una rampa. Con celdas de
ruido apenas mayores que el tile y 26 m de altura máxima, tiles contiguos podían diferir
decenas de metros: los claros quedaban al fondo de pozos rodeados de muros infranqueables,
invisibles desde fuera e inaccesibles a pie.

Corrección: altura máxima a 1000 cm, celdas de ruido a 60000/22000 cm (**decenas de tiles
por celda**, así el desnivel se reparte), exponente de 2.1 a 1.35 (también multiplica la
pendiente) y claros de 2400 cm.

**Lección codificada en un test**: `Astraeon.WorldGen.Terrain.Relief` ahora recorre 6 seeds
× 61 × 61 tiles y exige que el desnivel entre vecinos nunca supere
`GetMaxWalkableStepCm()`. Si alguien vuelve a subir la altura o a achicar las celdas, el
test falla antes de que el mapa se vuelva injugable.

### Rediseño: dos capas, y los marcadores se apoyan en el terreno (2026-09-06)

Segundo reporte del propietario: *"los relieves son muy altos como para caminar… de hecho
deberían haber montañas, pero hay que alisar sus capas"*.

El diagnóstico anterior era incompleto. El escalón **entre tiles** ya cumplía el límite,
pero había dos cortes a cuchillo que el test no miraba:

1. Los tiles por debajo de 60 cm no se instanciaban → el primer bloque de cada loma era un
   escalón de 60 cm contra el suelo desnudo.
2. Los tiles dentro de un claro se **descartaban por completo** → cada punto jugable quedaba
   en el fondo de un pozo con paredes del alto del terreno vecino.

Rediseño en **dos capas con propósitos distintos**, en vez de una curva que intentaba ser
paisaje y piso a la vez:

- **Suelo** (`GetGroundHeightCm`): amplitud contenida (260 cm) repartida entre decenas de
  tiles. Es piso y **siempre se camina**.
- **Montañas** (`GetMountainHeightCm`): formaciones escasas por umbral, hasta 52 m. **No se
  pretende escalarlas**: son barreras y puntos de referencia, se rodean a pie y se
  sobrevuelan con Ítaca. Quedan exentas del límite de escalón por diseño.

Y el cambio conceptual que resuelve los claros: **los marcadores ya no aplanan el terreno,
se apoyan sobre él**, consultando `GetGroundHeightCm` al materializarlos. Para eso se hizo
pura y consultable la función de altura. Sólo Ítaca conserva un llano —es una estancia
rígida de 8 × 6 m que no puede seguir el relieve— y su rampa usa un radio de 70 m, calculado
para que el descenso respete el escalón caminable.

Las montañas sí se cortan en seco cerca del juego: un corte brusco es aceptable porque una
montaña **ya es un muro** y se lee como pared de roca.

El test pasó a verificar tres invariantes separados: que el suelo sea caminable, que las
montañas **existan de verdad** (si nunca superan una loma, el paisaje volvió a ser plano y
el trabajo no cumple su propósito) y que el llano de Ítaca baje con rampa y no con
acantilado. Tamaño de tile bajado a 700 cm para que las laderas se escalonen más fino.

### Tres regresiones del terreno, y qué las causó (2026-09-06)

El propietario reportó tres fallos tras la tanda del relieve. Vale la pena dejar las causas
escritas porque las tres tienen la misma raíz: **cambios de terreno con efectos a distancia
que los tests no cubrían**.

1. **No se podía salir de la nave: pared de roca donde estaba la escotilla.**
   `MountainScale` se calculaba como `ComputeClearanceScale(...) > 0.0f`. `SmoothStep`
   devuelve un valor mayor que cero para **cualquier** distancia no nula, así que esa
   condición era verdadera en todas partes: **la exclusión de montañas nunca excluyó nada** y
   una montaña podía crecer justo sobre la puerta. Corregido con una comprobación explícita
   de radio (`IsWithinAnySpot`), ahora cubierta por test.

2. **Hueco gigante al despegar.** El llano **excavaba el suelo hasta Z=0** en un radio de
   70 m: un cráter, visible en cuanto la nave se elevaba. El radio era enorme justamente
   porque bajar 260 cm hasta cero exige mucha distancia para seguir siendo caminable.
   Corregido invirtiendo la idea: el claro es ahora una **meseta a la altura del suelo
   local** (`GetClearedGroundHeightCm`), con lo que el desnivel a repartir es mínimo y el
   radio pudo bajar a 26 m. Ítaca se posa sobre esa cota al empezar la partida y al aterrizar.

3. **Tirones al alternar pilotaje/aterrizaje y al iniciar la partida.** El terreno añadía
   unas 14.000 instancias **de a una** con `AddInstance`, y el componente recocina la
   colisión en cada llamada. Como la región se remateraliza al empezar y en **cada**
   aterrizaje, el tirón aparecía siempre. Ahora las transformaciones se acumulan y se
   entregan con `AddInstances` en un solo lote, y el campo se redujo de 420 a 320 m.

**Lección**: los dos primeros fallos existían en código que el test declaraba correcto,
porque el test comprobaba la *función de altura* y no el *terreno realmente construido*. El
`BuildTerrain` aplicaba lógica propia (recortes, escalas, umbrales) que ningún test tocaba.
Por eso ahora la nivelación vive en `GetClearedGroundHeightCm`, compartida por la
construcción y por los tests, para que no puedan divergir otra vez.

### Ajuste de parámetros del terreno

Todo vive en el namespace `AstraeonTerrain` de `AstraeonTerrainField.cpp`: `TileSizeCm`
(1200), `FieldRadiusCm` (48000), `MaxHeightCm` (2600), `FlatSpotRadiusCm` (1400) y las dos
frecuencias de ruido. Subir `MaxHeightCm` o bajar `TileSizeCm` encarece la colisión: medir
antes de empujarlos.

## 6.e Quinta tanda (2026-09-06): la supervivencia adquiere consecuencia

Auditando el código en busca de huecos de sistema aparecieron tres, y el primero era grave:

1. **La muerte no existía.** `HealthPercent` se clampeaba a 0 y no pasaba nada: se seguía
   jugando indefinidamente. Eso volvía **decorativo** todo lo construido alrededor de la
   supervivencia — daño ambiental, amenaza de criaturas, módulos de protección y el combate
   letal recién agregado no tenían desenlace posible.
2. **El oxígeno no se recargaba nunca.** Era una cuenta atrás sin contrajuego… que además no
   desembocaba en nada, por el punto 1.
3. **El mapa se revela y se persiste, pero no se puede mirar.** Pendiente.

Implementado para 1 y 2:

- Quedarse sin oxígeno **asfixia** (daño sostenido).
- Llegar a 0 de salud dispara el **rescate de emergencia de ARGOS**: el jugador vuelve a
  Ítaca, en pie pero no restablecido (`RestoreAfterRecall` deja 45 % de salud y 60 % de O2).
- **Estar dentro de Ítaca recarga** oxígeno y salud. La nave pasa a ser refugio, lo que le da
  un motivo mecánico a la base móvil que ya existía.
- El rescate **cuesta la carga opcional** (muestras de biomasa, vetas profundas) y **nunca**
  los insumos del recorrido crítico ni el equipo fabricado. Es la misma regla que ya aplica
  la mesa de fabricación: **el juego castiga lo opcional y protege el camino**. Perder el
  equipo obligaría a refabricar y podría dejar la partida sin salida.
- El HUD avisa en rojo (`[CRÍTICO — vuelve a Ítaca]`) y en azul dentro del refugio.

**45/45 tests.**

### Nota de trabajo en paralelo

Esta tanda se hizo mientras el agente de diseño trabajaba sobre `AAstraeonRegionMarker`
agregando meshes de blockout para recursos, vetas, señal y anomalía. Los cambios convivieron
sin conflicto (su versión conservó `RequiredToolId`), pero **conviene compilar tras cada
tanda** porque ambos tocan `Source/`.

## 6.f Sexta tanda (2026-09-06): construcción, inventario legible

**Sistema de construcción** — cadena completa `tierra → ladrillo → obra`:

- **Regolito**: `E` con el taladro apuntando al terreno. Se le dio un segundo uso a una
  herramienta existente en vez de inventar otra para el mismo gesto.
- **Ladrillos**: se fabrican en la mesa a partir de regolito.
- **Martillo de obra** (`B`) entra en modo construcción; **maza de demolición** derriba.
- **Vista previa**: fantasma translúcido que sigue la superficie apuntada, **verde si se
  puede colocar y rojo si falta material**. La respuesta llega antes de gastar, no después.
- `Q` cambia entre muro / plataforma / pilar, `R` rota 45°, click izq. coloca, click der.
  demuele. Demoler devuelve **menos** de lo que costó: rehacer no es gratis, pero corregir
  tampoco asusta.
- **Persistencia**: las obras se guardan **como datos** y se reconstruyen al materializar la
  región, porque la región se rehace entera cada vez que Ítaca aterriza. Vivir sólo como
  actores las habría borrado en cada vuelo.
- El tick del personaje pasa a cada frame **sólo** en modo construcción: 0,2 s alcanzan para
  supervivencia pero no para un fantasma que sigue la mira.

**Inventario legible**: cuatro categorías (materiales, construcción, herramientas, equipo),
nombres en castellano y cantidades alineadas. Los ids crudos se traducen **sólo para
mostrarse**: siguen siendo claves de guardado y de recetas, y no se tocan.

**Bug corregido de paso**: `StartSelectedNewGame` y `ContinueSavedGame` no verificaban
`bMenuVisible`, así que **pulsar `Enter` durante la partida reiniciaba la expedición** y
borraba el progreso en silencio.

**47/47 tests.** El de recetas dejó de contar cuántas hay (ese número crece con cada sistema
y obligaba a tocar el test sin motivo) y ahora afirma que las recetas imprescindibles
existen.

## 7. Próximos pasos sugeridos

1. **Cerrar el paso 1-2**: compilar, correr tests, empaquetar y probar el vuelo a mano.
2. **Paso 3 (generar planetas)** es el que le da sentido al vuelo: hoy la región es un cubo
   plano de 1200 m. Ver el contrato cube-sphere ya redactado en
   `ART_ASSET_MASTER_PLAN.md` §"Planetas, biomas y pruebas futuras" (radios Small=150 m a
   Giant=2000 m), que quedó preparado justamente para esto.
3. **Paso 4 (viaje interplanetario)** depende del 3.
4. Razas, civilizaciones y mesa de crafteo son cadenas de trabajo propias, no continuación
   de esta.

## 8. Pendientes heredados que siguen abiertos

- Piso de Ítaca superpuesto con la caja/paredes de `AAstraeonItacaInterior` (a cargo del
  trabajo de arte, en pausa). Ver `ART_ASSET_MASTER_PLAN.md` → "Hallazgo de smoke manual
  post-integración".
- ESCOTILLA no bloquea el paso físicamente (backlog, no bloqueante).
- Cierre del vertical slice: resolver la señal sólo imprime una línea en el HUD. El
  propietario decidió **no** invertir en un final ahora, porque "falta muchísimo al juego".
