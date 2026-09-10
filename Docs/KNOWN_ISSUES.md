## 2026-09-10 — Deuda de integración de Fase 2 tras P2.1/P2.2

- **Alta, abierta:** el runtime continúa con seis caras fijas; el servicio de workers todavía
  no está conectado a un selector quadtree/LOD. El constructor sí se comparte con `TL_11`.
  No usar esta iteración como evidencia de streaming jugable a 500 km.
- **Media, abierta:** faldón predeterminado de 100 cm sin dimensionamiento frente al error de
  LOD. Las muestras compartidas coinciden, pero eso no demuestra que el borde interpolado
  fino/grueso quede cubierto en una ruta visible. Medir en P2.3 antes de ajustar o hacer stitching.
- **Media, abierta (P2.3-A):** durante un relevo el conjunto visible puede superar el tope de
  la selección: 462 patches medidos contra un objetivo de hasta 384, porque conviven saliente y
  entrante. Además el delta de LOD visible puede ser 2 un instante, y el faldón está dimensionado
  para 1. Medir en `TL_12` (P2.3-C) conteo máximo, grietas y coste antes de acotar.
- **Baja, abierta (P2.3-A):** si el objetivo cambia más rápido de lo que se construye, un padre
  puede quedarse en pantalla mucho tiempo esperando a sus cuatro hijos. No hay agujero, pero sí
  detalle demorado. Medir el tiempo de asentamiento en el smoke de `TL_12`.
- **Media, heredada:** sigue pendiente el coste excesivo de reconstrucción de colisión
  documentado abajo. Corresponde a P2.4; no se altera el relevo validado de Fase 1.
- **Resueltos durante desarrollo:** exportar una clase propietaria de `TFuture` instanciaba
  copia implícita incompatible; se prohíbe copiar el manager. El primer test de skirts detectó
  un assert de `TArray::Add` al copiar desde el mismo array; se toman copias locales antes de
  añadir. Build posterior y las 85 pruebas pasan, sin desactivar aserciones.

## 2026-09-10 — Resuelto: la locomoción se cortaba 1,3 veces por segundo, y no era animación

Segunda prueba humana sobre `TL_11_CubeSphereClosed`. El propietario reportó que **el caminar se
ve trabado, como si el paso se cortara antes de terminar**, que al correr pasa lo mismo y que
además **avanza poco y se frena**. Su sospecha era la animación. No lo era.

**Lo que descartó el control.** Se instrumentó el smoke con `-AstraeonNoJump` —caminar sin saltar
ni una vez— y contadores de reparto de clips. Resultado sobre 40 s en línea recta a velocidad
constante:

~~~text
cayendo=0.0%   clip de aterrizaje=0.0%   clip Walk_F=99.0%   clip Idle=1.0%
cambios de clip=107 (2,67 por segundo)
transicion Walk_F->Idle = 53 veces
velocidad de animacion min=0 media=444 max=553
~~~

Ni caída, ni clip de salto, ni de aterrizaje: **el clip oscilaba entre `Walk_F` e `Idle` 53 veces
en 40 segundos**, y la velocidad de animación tocaba **exactamente 0**. El ciclo de paso dura
1,2 s y se reiniciaba ~1,3 veces por segundo, así que nunca llegaba a completarse.

**La causa.** `AAstraeonPlanetRuntime::PrepareCollision` rehacía la colisión cercana cada vez que
el jugador se alejaba `0.5/FaceQuads` rad del centro del parche —**312 cm** a 200 m de radio— y
para hacerlo usaba **un solo componente**: despegaba al personaje (`SetBase(nullptr)`), movía el
componente y recocía su cuerpo físico en el sitio. Durante esos frames el jugador se quedaba sin
suelo, `Velocity` caía a 0 y el selector devolvía `Idle`. Nunca llegaba a `Falling`, que es por lo
que todas las pruebas anteriores pasaban en verde: el defecto vivía justo por debajo de lo que
medían. La correlación es 1:1 — **55 reconstrucciones de colisión, 53 cortes de animación**.

Por eso empeoraba al correr: el umbral es de distancia, no de tiempo. A 520 cm/s toca cada 0,57 s;
a 900 cm/s, cada 0,35 s.

- **Corregido**: doble búfer de colisión. El relevo se construye completo y se activa **antes** de
  retirar el saliente, y al personaje se le pasa la base de uno a otro en vez de dejarlo sin
  ninguna (`AstraeonPlanetRuntime.cpp`, `PrepareCollision`).
- **Medido después**, mismos 40 s: `cambios de clip=1` (el arranque `Idle->Walk_F` y nada más),
  `Walk_F=100,0%`. Corriendo: `Run_F=99,2%` y sólo las dos transiciones de arranque.
- **Y no era sólo visual**: el recorrido en 40 s caminando pasó de **16.922 cm a 20.934 cm**, un
  **24 % más**. El "avanza poco y se frena" era literal; cada corte costaba distancia real.
- **Guardián permanente**: el smoke ahora falla si más del 0,5 % de los frames caen en `Idle`
  mientras camina. El defecto medía ~1,0 %; el arreglo mide 0,00 %.
- **Batería completa tras el arreglo**: `Automation` 75/75, `Cardinals` PASS, `Walk` de 250 s
  `RESULTADO=OK` con 141.247 cm y **125 cambios de clip que son exactamente 62 saltos × 2 + 1
  arranque**: ni un corte espurio.

**Consecuencia para el pulido de brazos.** La queja de que *"el caminar no se lee natural"* tenía
dos causas sumadas, y ésta era la ruidosa: un ciclo que se reinicia tres veces por vuelta se ve
antinatural con cualquier pose. La abducción de 9,5° sigue siendo un defecto real y medido, pero
**el juicio sobre los codos hay que rehacerlo ahora que el ciclo se reproduce entero**, antes de
gastar otra pasada de Blender.

**Deuda registrada, no bloqueante.** El parche de colisión abarca `6.0/FaceQuads` rad (~37 m a
200 m de radio) pero se rehace cada `0.5/FaceQuads` (~3,1 m): doce veces más seguido de lo que su
propio tamaño exige, recorriendo las 6.144 celdas de las seis caras y recociendo cada vez. Ya no
se ve, pero es trabajo desperdiciado. Corresponde a la **Fase 2**, que es la dueña del anillo de
colisión y del streaming por patches.

## 2026-09-10 — Prueba humana: el pulido de brazos no convence y aparecen tres huecos de diseño

El propietario jugó la variante integrada y devolvió cuatro observaciones. Sólo una es un defecto
de algo que se declaró hecho; las otras tres describen trabajo que **nunca se hizo** y que la
prueba puso a la vista.

**1 — Los codos siguen pegados al cuerpo; el caminar no se lee natural.** No está resuelto.
Medido sobre `Tools/Blender/main_character_anim.py`: `STAND` fija `upperarm_* X = -1,080 rad` y el
brazo colgando en reposo es `-1,245` (`reach(0)`), así que la separación real del tórax es de
**0,165 rad ≈ 9,5°**. Con la mochila y el volumen del traje encima, esa abducción se lee como
brazos pinzados. Peor: `clavicle_l/r` sólo se toca dentro de `reach()`, o sea que en `Idle`,
`Walk_*` y `Run_*` **los hombros nunca se abren** y toda la anchura de la silueta depende de ese
único ángulo. El valor está escrito a mano **12 veces** en el archivo, de modo que subirlo en
`STAND` no alcanza: cada clave de ciclo lo vuelve a escribir encima. La corrección es una
constante de módulo más apertura de clavícula, no una edición puntual.

**2 — No existe quitarse el casco.** Ni malla sin casco, ni animación, ni acción que la dispare.
Hoy `Helmet` es una de las tres piezas fijas de equipo montadas en
`AstraeonPlayerCharacter.cpp:158`. El documento rector ya lo tenía previsto: *personaje con casco*
y *personaje sin casco* son entregables literales de la **Fase 5**.

**3 — La mano de primera persona no es la del personaje.** La observación es correcta y el
problema es estructural, no un ajuste de material. Primera persona usa
`SK_Human_HandsFP_Blockout` sobre `SKEL_Humanoid_A` (57 huesos, blockout); el cuerpo usa
`SK_Astraeon_Player_Skeleton` (75 huesos, 65.284 tris, 4 LOD, sets de textura
Character/Suit/Gear/Helmet). Son dos assets de dos generaciones distintas conviviendo. Esa
divergencia es la causa de `BodyCounterpart()` y de que el pulido de locomoción haya tenido que
hacerse **dos veces**, una por juego de clips. Unificar primera persona sobre los brazos del
protagonista borra la duplicación entera, no sólo el síntoma visual.

**4 — El escáner aparece en la mano en primera persona y no en tercera.** Es consecuencia directa
del punto anterior. `ToolMesh` cuelga de `socket_tool_r` **del rig de manos**
(`AstraeonFirstPersonRigComponent.cpp:71`) con `SetOnlyOwnerSee(true)`, y con las manos vacías
muestra el escáner por diseño (`:409`). El cuerpo de tercera no lleva herramienta porque
`socket_tool_r` **no existe en su esqueleto**: lo crea `Tools/Blender/generators/tools_blockout.py`
sobre el rig blockout. Ponérsela hoy al cuerpo no es enganchar un componente, es **medir un
transform de agarre nuevo** contra `hand_r` del esqueleto del protagonista, y ese trabajo se tira
a la basura al unificar. Por eso no se parchea suelto.

Reparto por fases en `BACKLOG.md`: el punto 1 es tarea acotada de la Fase 1 en curso; los puntos
2, 3 y 4 son **Fase 5 — Protagonista, Ítaca y vuelo**, que ya los declara como entregables.

## Estado actualizado 2026-09-10 — pulido de locomoción integrado

La primera pasada de pulido de brazos y salto ya está guardada, exportada e integrada en el
runtime mediante `/Optimized_Polished`. Las pruebas de editor y build pasaron. Aún no se ha
cerrado la medición cuantitativa de patinaje de pies ni la validación humana; no declarar esos dos
aspectos como resueltos hasta completarlos.

Corrección posterior: la observación del propietario era válida para primera persona. La pasada
anterior sólo había pulido el cuerpo de sombra; ahora el runtime usa clips FP aislados con
balanceo de brazos. Evidencia: `ContentPipeline/reports/polished_first_person_hands_validation.json`.

## 2026-09-09 — Resuelto: el salto se trababa al moverse en el aire

Segunda prueba manual del harness. La cámara y la vuelta al mundo quedaron bien; el propietario
reportó que **al saltar y desplazarse en el aire la animación de salto se trababa y el personaje
patinaba**.

**No era animación: el personaje nunca aterrizaba.** Medido con `-AstraeonSmokePlanetWalk`:
en caída el **98,3 % del tiempo**, racha continua de **116 segundos**, a altitud constante de
96 cm y con velocidad puramente tangencial. El clip de salto se quedaba puesto porque el estado
de movimiento seguía siendo `Falling`.

**Causa.** Al desplazarse tangencialmente sobre una superficie **convexa**, el contacto con la
esfera es rasante: el impacto cae en el borde de la semiesfera inferior de la cápsula y
`IsValidLandingSpot` lo descarta —correctamente para su propósito original— como roce de pared y
no como aterrizaje. El personaje quedaba deslizándose con la velocidad radial recortada contra la
superficie, sin volver nunca a `Walking`.

El experimento que lo aisló: **saltando quieto aterriza siempre** (racha máxima 1,01 s);
**saltando en movimiento no aterriza nunca**. Todo lo demás estaba bien y se descartó midiendo:
gravedad −980 con dirección radial exacta, colisión presente (un barrido propio encontraba la
esfera a 54 cm con normal 1,000), y el orden de tick no influía (A/B con `TG_PostPhysics` dio el
mismo 97,7 %).

- **Corregido**: `UAstraeonPlanetGravityComponent::GroundIfRestingOnSurface()` re-apoya al
  personaje cuando está sobre suelo caminable pero el motor lo tiene por en el aire. Sólo actúa si
  no está subiendo, si hay superficie dentro del margen bajo los pies y si su pendiente es
  caminable según el propio umbral del motor.
- **Medido después**: racha máxima en el aire **1,02 s**, caída 25,6 % con 37 saltos en 148 s
  —justo lo que suman los saltos—, y el clip de salto acompaña.

**Dos suposiciones de Z corregidas de paso**, ambas prohibidas por el ADR 0004 §4.1:

- `FAstraeonBodyAnimationState::SpeedCms` usaba `Velocity.Size2D()`, o sea el plano XY del mundo.
  En el ecuador el arriba local es X, así que esa cuenta metía la velocidad vertical dentro de la
  horizontal. Ahora es la velocidad **tangencial** proyectada contra el arriba del actor, que da
  el mismo resultado en el mapa plano. Igual en la selección de clip de las manos.
- `RescueFromVoidIfNeeded` comparaba `Z` de mundo para decidir si el jugador se había caído. En el
  hemisferio sur "abajo" es Z creciente, así que el rescate quedaba ciego justo en media esfera.
  Ahora mide la caída a lo largo del arriba local.

## 2026-09-09 — Resuelto: los cuatro defectos de la primera prueba manual del harness

El propietario probó `TL_10_RadialGravity` a mano y reportó cuatro cosas. Las cuatro tenían
causa distinta.

**1 y 2 — El personaje se volcaba boca abajo al caminar; la cámara se invertía a cierta
inclinación.** Misma causa: `bUseControllerRotationYaw` está en `true` por defecto en `APawn` y
las cámaras usaban `bUsePawnControlRotation`. El motor escribía la rotación del actor desde una
**`FRotator` de mundo** cada frame y el componente planetario la sobrescribía con el marco local:
dos sistemas peleándose por la misma rotación. Cerca del polo coinciden; al alejarse, el "arriba"
del mundo deja de ser el del jugador.

- **Corregido**: el marco de mirada pasa a vivir en la gravedad local. El yaw es un giro alrededor
  del arriba local, el frente se **transporta** al nuevo plano tangente en vez de recalcularse
  desde un eje fijo, el pitch es una inclinación relativa de la cámara con tope a 85°, y el
  componente apaga las banderas de rotación de mando mientras dure la gravedad planetaria.
- **Cubierto por**: `Astraeon.Planet.Frame.TransportKeepsHeading`,
  `Astraeon.Planet.Frame.YawIsAroundLocalUp` y el smoke `-AstraeonSmokePlanetWalk`, que da una
  vuelta completa a la esfera con **0,0 % de frames desalineados**.

**3 — Pantalla negra al empezar partida.** Regresión introducida el mismo día: el guardián que
evita materializar la región plana en un mapa planetario se llevó por delante
`EnsureRuntimeLighting()`. Las luces que coloca el script del mapa son **estáticas** y sin
lightmap horneado no iluminan; las que crea el runtime son movibles y funcionan sin hornear.

- **Corregido**: la luz se enciende siempre. Sólo se saltan la región plana y la estancia de Ítaca.

**4 — Deslizamiento sin control tras saltar.** El componente tickeaba en `TG_PostPhysics`, con el
razonamiento de que alinear después del movimiento usa la posición final del frame. Está al
revés: dejaba que `CharacterMovement` resolviera el suelo con la orientación y la gravedad del
frame **anterior**. El desfase de un frame en la posición es invisible; el desfase en la
orientación con la que se busca el suelo, sobre una superficie curva, se siente como resbalar.

- **Corregido**: `TG_PrePhysics` más una dependencia explícita de tick, para que
  `CharacterMovement` no corra hasta que la gravedad y la orientación del frame estén puestas.
- **Medido**: 2,0 % de frames en caída en una vuelta completa, y son el salto.

**Hallazgo de paso, ya corregido.** El smoke se detenía siempre a los 89,7° de arco. No era un
defecto del movimiento: era la baliza del ecuador, que caía exactamente sobre el meridiano que
recorre quien camina de frente desde el polo. Y el primer intento de quitarle la colisión no
persistió, porque `set_collision_enabled` altera el estado en memoria y no la `BodyInstance` que
se serializa — hay que cambiar el **perfil**. El generador del mapa ahora relee tras guardar y
falla si alguna baliza sigue bloqueando.

**Sigue abierto, y sólo lo cierra una partida**: si la cámara *se siente* bien. Que no ruede y no
se invierta está medido; que acompañe al jugador sin marearlo, no.

## 2026-09-09 — Abierto: `-nullrhi` mata al editor al spawnear un actor propio con malla

Encontrado generando `TL_10_RadialGravity` (Fase 0). Cualquier script Python que llame a
`EditorLevelLibrary.spawn_actor_from_class` con **una clase propia que tenga componente de malla**
mata el editor con `EXCEPTION_INT_DIVIDE_BY_ZERO` si se arrancó con `-nullrhi`.

- **No es específico del código nuevo.** Se comprobó con un control: `AAstraeonRegionMarker`, que
  lleva meses en el juego, crashea exactamente igual. `PlayerStart` no. La primera hipótesis —el
  constructor del harness— era falsa y costó cuatro iteraciones descartarla; el control debió ser
  el primer experimento, no el quinto.
- **Por qué no había aparecido antes.** `CreateBootstrapMap.py` y `CreateItacaArtTestMap.py` sólo
  spawnean clases del motor (`PlayerStart`, `DirectionalLight`, `SkyLight`) y usan
  `spawn_actor_from_object` para las mallas. Nunca spawnean una clase de Astraeon.
- **No afecta a las pruebas.** `RunCharacterChecks.ps1 -Check Automation` usa `-nullrhi` y sus 65
  pruebas pasan: el fallo está en la ruta de spawn *del editor*, no en la del juego.
- **Solución aplicada**: los scripts que spawnean clases propias usan `-RenderOffscreen`. Queda
  documentado en la cabecera de `CreateRadialGravityTestMap.py`.
- **Sin diagnosticar**: la causa raíz dentro del motor. El callstack no tiene ni un frame de
  Astraeon —es `EditorScriptingUtilities` → `UnrealEd` → `Engine`— y los símbolos no resuelven.
  No se investiga más porque el workaround es de una palabra y no bloquea ninguna fase.

## 2026-09-09 — Trampa: `new_level` crea el nivel pero el mundo activo puede no cambiar

Misma sesión. `EditorLevelLibrary.new_level(ruta)` crea y guarda el `.umap`, pero bajo
`-RenderOffscreen` el mundo activo del editor puede seguir siendo el mapa por defecto. Los
`spawn_actor_from_class` posteriores caen entonces en **`L_AstraeonBootstrap`**, y un
`save_current_level()` detrás lo sobrescribiría.

Se detectó en una sonda que no guardaba, así que no hubo daño: `L_AstraeonBootstrap.umap` quedó
verificado sin cambios contra HEAD. `CreateRadialGravityTestMap.py` ahora llama a
`require_current_level()` justo después de `new_level` y aborta si el mundo activo no es el
esperado. Todo script nuevo que cree un mapa debe hacer lo mismo.

## 2026-09-09 — Resuelto en su mayor parte: los clips del cuerpo que no se reproducían

Levantado el 2026-09-08 por la auditoría de arte: el runtime reproducía **5 de los 45**
clips del protagonista.

- **Resuelto**: `FAstraeonBodyAnimation::Choose` engancha 18 clips —direcciones de caminar
  y correr, las tres fases del salto y los gestos de escanear, interactuar, recoger, usar
  herramienta, comer y presentar—. Caminar de lado ya no usa el clip de caminar de frente.
  Cubierto por `Astraeon.Art.Character.BodyAnimationSelection` y por la prueba del rig.
- **Sigue pendiente, con nombre**: los clips de agachado (`Crouch_*`) no tienen mecánica que
  los dispare —agacharse no existe en el juego—, y las poses de porte (`OneHand_*`,
  `TwoHand_*`, `Grip_*`) necesitan mezcla por capas sobre la locomoción, que la reproducción
  de un solo nodo no da.
- **Detalle**: `Docs/CAMARA_Y_MANOS.md` → "El cuerpo usa su set de animación".

## 2026-09-08 — Duplicado: el import crudo del protagonista

- **Motivo**: `/Game/Astraeon/Characters/Player` conserva el import previo (47 assets,
  incluidos malla, esqueleto y 45 clips) que quedó superado por
  `/Game/Astraeon/Characters/Player/Optimized`.
- **Impacto**: ninguno en juego —no se cocina— pero pesa en el repositorio y confunde al
  buscar el asset bueno. Su único slot de material apunta a un atlas que ya no existe.
- **Cierre esperado**: decidir si se conserva como fuente de reimportación o se elimina.
  Mientras tanto, `AuditArtUsage.py` lo reporta aparte para que no oculte un defecto real.

## 2026-09-08 — Pendiente: la animación del protagonista se ve rara

Reportado por el propietario tras probar el ejecutable, ya con el personaje visible:
**camina raro, salta raro y los brazos se ven mal**. No era un fallo funcional —el personaje
se veía y el recorrido crítico pasaba— sino calidad de animación.

- **Causa corregida**: los clips locomotores tenían la pose base demasiado cerrada para el
  volumen de las hombreras. Se reautorizaron en Blender `Idle`, `Walk_*` y `Run_*` con pose A
  (`upperarm X = -1,08 rad`), dejando separación visible brazo/tórax sin convertirla en T.
- **Causa histórica, ya medida**: los 45 clips del cuerpo posan **los brazos en cruz**, no al
  costado ni al frente. El eje que lleva el brazo al frente en este rig es Z y los clips lo
  usan entre 0,13 y 0,46; en `TwoHand_Idle` las manos quedan a **x = ±0,48 m** del cuerpo.
  La medición está en `PENDIENTE_PROTAGONISTA.md` §"Por qué se eligió convivir con dos
  esqueletos". El set nunca tuvo una pasada de pulido: se autorizó para validar el pipeline.
- **En primera persona, corregido**: `PolishedFP/AN_HandsFP_Polished_*` conserva el agarre del
  escáner pero añade balanceo medido en Walk/Run y fases de Jump/Land.
- **Corte de locomoción, corregido en runtime**: el selector tenía cambios instantáneos y
  reiniciaba el ciclo al cruzar Idle/Walk/Run o al cambiar dirección. Ahora usa histeresis y
  conserva la fase normalizada al pasar entre ciclos.
- **El salto ya no es un clip suelto** (2026-09-09): despegue, vuelo y aterrizaje están
  encadenados. Sigue **sin medir** el patinaje de pies en el ciclo de caminata: velocidad del
  clip contra velocidad del `CharacterMovement`.
- **Impacto**: estético y constante — se ve en cada partida, en tercera persona sobre todo.
- **Cierre esperado**: re-autorizar en Blender las poses de brazo del set de locomoción,
  revisar contacto de pies contra la velocidad real de movimiento, y separar el salto en
  despegue/vuelo/aterrizaje. Reexportar e importar: la normalización de escala de raíz ya
  está enganchada a los scripts de importación, así que no hay que repetir aquella
  reparación.

  **Avance 2026-09-10:** Blender quedó guardado, el FBX fue reexportado e importado en
  `/Optimized_Polished`, el build Development se regeneró y pasaron Automation 75/75,
  Critical, Package, PackagedCritical y PackagedVisual. Queda únicamente medir patinaje de
  pies contra `CharacterMovement` y confirmar con una prueba humana sostenida.

## 2026-09-07 — Generación del protagonista: estimador ausente

Conexión con Blender resuelta y edición probada. La API ofrece `bl_generate_3d` y el
catálogo incluye Tripo/Meshy, pero no expone `bl_estimate_generation` ni la opción
`include_schema` que cita la skill `blender-generation`. Esa guía prohíbe enviar un job
para descubrir su costo. Solicitud concreta preparada, sin enviar; excepción del propietario
para una generación de costo no estimable o construcción nativa pendiente de respuesta.
El proxy nativo v02 se guardó; no satisface calidad final, rig, materiales ni animación.

## 2026-09-07 — Resuelto: acceso Bridge (diagnóstico previo)

Reinspección vigente: el servidor `mcp__higgsfield_bridge__` responde y expone herramientas
`bl_*`, pero dos consultas `get_host_status` devuelven `blr:false`. La inspección real falla:
`Blender is not connected. Open the Higgsfield panel in Blender and press Connect in "Supercomputer Connection".`
Consulta inicial de procesos locales: lista Blender vacía. Luego se abrió con autorización,
PID 11784 responde, pero el Bridge mantiene `blr:false`. Vías comprobadas: estado de hosts,
inspección de escena y procesos. Conectar ese panel; luego repetir inspección.
El HTTP 401 del POST anónimo anterior es histórico y no describe el bloqueo vigente.
No afecta al humano Q1 existente. Detalle en
`graphics/characters/main_player/docs/MAIN_CHARACTER_PIPELINE.md`.

## 2026-09-06 — Vista que gira y suelo que patina: la base de movimiento

**Síntoma clave**: al saltar el problema desaparece y al caer vuelve. Eso descarta el
renderizado y señala la **base de movimiento**: en el aire no hay base, en el suelo sí.

`UCharacterMovementComponent::bIgnoreBaseRotation` viene en `false` por defecto, así que el
Character **hereda la rotación de aquello sobre lo que se apoya**. La criatura llama a
`SetActorRotation` en cada frame para mirar hacia donde se mueve, bloquea, mide 70 cm de alto
y desde que su envolvente se apoya en el suelo es pisable. Pararse encima hacía girar la
vista sola y arrastraba al jugador ("patina el piso y me lleva a otra superficie").

Corregido en tres frentes:

- `bIgnoreBaseRotation = true` en el Character: una vista en primera persona no debe rotar
  nunca por culpa del suelo, apoye donde apoye.
- `CanCharacterStepUpOn = ECB_No` en la envolvente de la criatura: no se puede subir encima
  de un animal, y así deja de poder ser base de movimiento.
- La criatura sigue moviéndose sin barrido, así que **todavía puede atravesar paredes**; ver
  pendientes abajo.

**Sin confirmar**: el usuario también lo reportó "en un borde de la nada" y fuera de la nave.
Si con esto persiste, la build trae `-AstraeonDiag`: la línea `blockingOverlaps` distingue
entre quedar atrapado entre superficies (dos o más) y otra causa (cero).

## 2026-09-06 — Resuelto: las "paredes" que rodeaban Ítaca al iniciar

No eran paredes: eran las 12 rocas procedurales. Su anillo de 3,5–14,5 m se centraba en el
**origen del mundo** en vez de en Ítaca, así que al empezar la partida rodeaban la nave. Su Z
tampoco consultaba el terreno —se calculaba contra Z=0—, de modo que sobre relieve quedaban
medio enterradas y se leían como losas planas. Databan de antes del campo de relieve, que ya
aporta el paisaje que ellas intentaban dar. **Eliminadas.**

## 2026-09-06 — La escotilla desde afuera ya no despliega a la superficie

Interactuar con la ESCOTILLA estando fuera teletransportaba a la plataforma de superficie,
donde el jugador ya estaba: era redundante. Ahora la escotilla funciona como puerta en los
dos sentidos y desde afuera devuelve a la estancia.

## Pendiente — la criatura atraviesa la geometría

`AAstraeonCreatureActor::Tick` mueve con `SetActorLocation` sin barrido, así que persigue al
jugador a través de las paredes de Ítaca y de cualquier relieve. Ya no puede arrastrarlo ni
girarle la vista, pero verla entrar por una pared sigue siendo un fallo visible. Moverla con
barrido es el arreglo, con el riesgo de que quede trabada contra el terreno: necesita prueba.


## 2026-09-06 — RESUELTO: el relieve atravesaba la estancia de Ítaca (causa raíz)

Síntomas reportados, todos con la misma causa: piso rocoso en vez de la cubierta de Ítaca,
"paredes" alrededor de la nave, la vista que se vuelve loca al moverse, y el rescate que
devuelve al jugador a la nave. **Ocurría ya al iniciar partida, sin despegar**, lo que
descartó el aterrizaje como causa.

**Causa.** Las baldosas de relieve miden 7 m, nacen en Z=0 y suben hasta la altura del suelo.
Ítaca se posaba exactamente a esa misma altura, así que:

- la baldosa central quedaba con su cara superior **en el mismo Z** que el piso de la
  estancia — z-fighting, y ganaba la roca: de ahí el "piso rocoso";
- las baldosas vecinas, a 7 m, quedaban hasta ~46 cm más altas y **entraban dentro de la
  huella de 8 × 6 m** — de ahí las "paredes" alrededor;
- el jugador quedaba atrapado entre el piso de la estancia y un bloque de roca, y la
  despenetración de la cápsula lo empujaba: de ahí la vista descontrolada, y la caída
  posterior que disparaba `RescueFromVoidIfNeeded`.

**Corrección.** Tres piezas que ahora comparten una única fuente de verdad,
`AAstraeonTerrainField::GetItacaPadHeightCm()`:

1. Las baldosas que pisan la huella de Ítaca —más media baldosa de margen, porque una
   baldosa de 7 m entra por el borde aunque su centro caiga fuera— se aplanan a la cota de
   la plataforma y no reciben montaña.
2. Ítaca se posa `GetDeckThicknessCm()` (12 cm) **sobre** esa plataforma, no a su misma cota,
   tanto al iniciar partida como al aterrizar. El umbral de la escotilla queda en 12 cm,
   muy por debajo del escalón caminable de 45 cm.
3. El jugador aparece relativo al origen de Ítaca y no en un Z=100 fijo del mundo, que lo
   dejaba dentro del bloque de terreno.

Cubierto por `Astraeon.WorldGen.Itaca.DeckRestsOnTerrain`, que comprueba sobre seis seeds que
la cubierta se apoya sobre la plataforma, que la plataforma nunca queda por debajo del umbral
de instanciado y que el umbral de la escotilla sigue siendo caminable.

## Resuelto — la mesa de fabricación se quedaba en el suelo al despegar

`SetItacaInteriorHidden` enumeraba tres piezas y omitía `itaca_fabricator`. Ahora pregunta
por `AAstraeonRegionMarker::BelongsToItacaInterior()`.

## 2026-09-06 — Bugs de vuelo reportados en sesión: dos corregidos, uno sin causa confirmada

Reporte del propietario tras jugar la build `WindowsItacaExterior`:

1. Al despegar, **la mesa de fabricación se queda en el suelo del mapa**, fuera de la nave.
2. Al aterrizar, a veces "se pone loco": la vista **gira** al moverse y el suelo se ve
   **"entre líneas"**; el giro continúa fuera de la nave y de golpe **te devuelve a la nave**.
3. Saltando y moviéndose, en algún momento deja de pasar.

### Corregido — la mesa de fabricación quedaba atrás (causa confirmada)

`AAstraeonGameModeBase::SetItacaInteriorHidden` enumeraba `itaca_argos_console`,
`itaca_pilot_console` y `itaca_surface_hatch`, y **omitía `itaca_fabricator`**. La mesa nunca
se ocultaba ni se le quitaba la colisión al despegar. Ahora se pregunta por
`AAstraeonRegionMarker::BelongsToItacaInterior()`, de modo que la próxima estación que se
añada no vuelva a quedarse afuera por omisión.

### Corregido — piso de la estancia coplanar con el terreno (causa confirmada por lectura)

El plato de región se crea a Z −52 con la cara superior en −2, y el comentario del código
declara la suposición: *"Region top is -2 cm, below the cabin floor at 0"*. Eso vale mientras
Ítaca vive en Z=0. Al aterrizar, `RequestShipLanding` posaba la estancia exactamente en
`GetGroundHeightCm(...)`, que llega a 260 cm, y el bloque de terreno bajo la nave se aplana a
**esa misma cota**: la cara superior del piso de la estancia y la del bloque de terreno
quedaban en el mismo Z. Eso produce z-fighting —el "entre líneas"— y, peor, dos superficies
de colisión coincidentes bajo los pies, que es la explicación habitual de empujes y vibración
que parecen aleatorios. Ítaca ahora se posa `GetDeckThicknessCm()` (12 cm) por encima del
terreno, así que su cubierta se apoya encima en vez de atravesarlo. El umbral en la escotilla
queda en 12 cm, muy por debajo del escalón caminable de 45 cm.

### Sin causa confirmada — la vista que gira

No se identificó por lectura de código qué rota la vista. La hipótesis es que sea consecuencia
del punto anterior —una cápsula atrapada entre dos superficies coincidentes se despenetra a
empujones—, y el rescate que "te devuelve a la nave" es
`AAstraeonPlayerCharacter::RescueFromVoidIfNeeded`, que dispara tras caer 20 m y teletransporta
al último suelo seguro, que después de aterrizar es el interior. **Esto no está verificado.**
Para eso se añadió la instrumentación de abajo.

## Instrumentación de sesión — `-AstraeonDiag`

```powershell
& "C:\Users\MAXIMO\Desktop\Astraeon\Builds\WindowsDiag\Astraeon.exe" -AstraeonDiag -abslog="C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\Sesion.log"
```

Escribe en `LogAstraeonDiag`, a 5 Hz: posición, velocidad, modo de movimiento, rotación de
control y del actor, origen de Ítaca, si el jugador está dentro de la huella de la estancia,
altura analítica del terreno, qué actor y componente hay bajo los pies, y —lo que más importa—
**cuánta geometría bloqueante solapa la cápsula**. Cero es lo normal de pie; dos o más
superficies atravesando la cápsula es exactamente el caso que produce el empuje.

`LIFTOFF`, `LANDING` y `RESCUE` se registran **siempre**, con o sin la bandera: son los tres
momentos en que el mundo se reconstruye o el jugador es teletransportado.

Los mensajes en pantalla no sirven para esto: el GameMode ejecuta `DisableAllScreenMessages`
en BeginPlay, así que `AddOnScreenDebugMessage` es un no-op silencioso.


## 2026-09-06 — Exterior de Ítaca Q1, límites (severidad baja)

- **Sin registro entre exterior e interior (media):** el hueco de la escotilla no está
  modelado en el casco, y no hay cabina ni ventana. Interior y exterior siguen siendo dos
  actores que nunca se ven a la vez.
- **La hoja no cierra sola (baja):** una vez abierta queda abierta durante la sesión y el
  estado no se guarda. Cerrarla al volver a entrar es una decisión de diseño pendiente.
- **Sin animación de despegue ni aterrizaje (baja):** la estancia se oculta y aparece la nave,
  como antes. Los patines no se repliegan y los motores no tienen ciclo; ambos ya tienen
  anclaje donde colgar esos VFX, pero nada de eso está hecho.
- **Silueta deliberadamente roma (baja):** caras planas y utilitarias, para no chocar con las
  paredes de cajas de la estancia. El perfil del casco es una sola lista de puntos en
  `Tools/Blender/configs/ship_blockout.json` si se quiere algo más esbelto.

## Resuelto — la ESCOTILLA funcionaba como puerta siempre abierta (2026-09-06)

El propietario había reportado que el marco no bloqueaba el paso: se podía salir de Ítaca
caminando por el hueco sin pulsar `E`, y `E` sólo disparaba el teletransporte. Ahora hay una
hoja real sobre bisagra que empieza cerrada y bloquea, y que gira 95° hacia dentro al usar el
marcador. La caja de la hoja ignora el canal de visibilidad a propósito: frena a la cápsula
pero deja pasar el trazo de interacción, porque bloquear también el trazo habría dejado la
escotilla imposible de abrir.


## 2026-09-06 — Criatura Q1, límites de presentación (severidad visual media-baja)

- **Amenaza poco contrastada (media):** el gesto de `Threatening` —cabeza baja, mandíbula
  abierta, peso atrás— se distingue del reposo, pero es sutil; como aviso a distancia puede
  no leerse. Reproducir: `ContentPipeline/Generated/CreatureBlockout/Preview_Threaten.png`
  contra `Preview_Base.png`.
- **Recule exagerado (baja):** `Hit` levanta la cabeza hasta 1,07 m en un animal de 0,71 m.
  Legible, pero en el límite de lo plausible.
- **Sin IK ni mezcla (baja):** los clips cortan, la marcha es un trote diagonal y el galope
  de huida conserva un apoyo, sin fase de vuelo refinada. Mismo límite que el lote humano.
- **Cadáver sin física (baja):** la muerte es un clip, no un ragdoll. Sin PhysicsAsset, sin
  colisión ajustada a la silueta, sin VFX ni audio de impacto o pisadas.
- **Variación no paramétrica (baja):** el contrato determinista por `EntitySeed` del plan
  maestro sigue sin implementarse; hoy la variante es una elección entre dos mallas.


## 2026-09-06 — Encuadre del rig de primera persona, sin confirmar (severidad visual media)

Las manos, la herramienta y la sombra propia ya están montadas y verificadas por test
(`Astraeon.Art.Character.FirstPersonRigIsWired`), pero **la composición en pantalla no la
vio nadie todavía**. Tres números están derivados de la geometría, no medidos en cámara:

- `HandsOffsetCm` (0, 0, -160) y `HandsRotation` (0, -90, 0): sitúan el ojo del rig en la
  cámara. En pose A los brazos cuelgan a los lados, así que es esperable ver poco las manos
  en reposo y caminando, y verlas bien en los gestos. Ambos son `EditAnywhere`.
- `ToolGripTransform`: conversión de la matriz de agarre de Blender al marco de Unreal
  (conjugar por diag(1,-1,1) y transponer). La matemática cierra —ortonormal, det +1— pero
  si la herramienta aparece girada o desplazada en la mano, este transform es el sospechoso.
- Eje de giro de la broca del taladro: se asume el pitch de Unreal.

Reproducir: jugar y mirar las manos en reposo, caminando, corriendo, y al escanear,
disparar, taladrar e interactuar. Ver `CHARACTER_TOOLS_INTEGRATION.md`.

## Resuelto — import de herramientas bloqueado (2026-09-06)

`ContentPipeline/reports/tools_unreal_import.json` quedó en `passed: false` con
`Grip-origin bounds mismatch`, dejando el lote entero sin manifiesto ni integración.
No era el arte: `ValidateToolsImport.py` comparaba el origen de bounds de Unreal contra el
centro de Blender sin aplicar el espejo en Y que introduce `convert_scene`. La comprobación
de dimensiones pasaba porque son absolutas; la de origen no, porque el signo de Y sí importa.
Medido tras el fix, escáner: `origin_cm.y = +3.0` contra `-3.0` en Blender. El chequeo ahora
niega Y y registra `origin_cm`/`expected_origin_cm` por asset.

## Resuelto — dos tests obsoletos de la fase de mundo vivo (2026-09-06)

`Astraeon.WorldGen.Region.Invariants` esperaba exactamente 3 POIs y encontraba 7;
`Astraeon.Art.Region.PresentationPreservesGameplay` descartaba el id antiguo
`first_mob_patrol_origin`. Ambos rotos por `AstraeonScience::CreatureSpawnCount = 5`. Se
reemplazó el conteo mágico por una comprobación de composición y el descarte por prefijo
`creature_spawn`. Sin cambios de gameplay.


## 2026-09-06 — Límites del prototipo humano Blender (severidad visual media)

- Cuerpo, manos y siete clips existen y pasan importación UE transitoria, pero todavía
  no están conectados al Character. Reproducir: abrir la build jugable anterior;
  continúa usando la presentación previa. Ver `HUMANOID_ART.md`.
- En previews de guantes, los puños y algunas protecciones muestran transiciones
  bruscas entre superficies superpuestas. La anatomía y materiales son Q1.
- Marcha/carrera son clips en el sitio, con apoyo de la bota inferior; falta IK y
  sincronización con velocidad para reducir deslizamiento. Carrera sin fase de vuelo
  refinada; salto sin trayectoria vertical de root, que corresponde al Character.
- El agarre es una prueba de cierre de dedos; no certifica contacto con herramientas
  todavía no modeladas ni encuadre desde la cámara FP real.

Estas limitaciones no se ocultaron mediante un cambio de colisión o de controles.

## Arte regional Q1 — 2026-09-06

- **Presentación pendiente (media):** siete meshes integrados, todavía facetados y con un
  material plano. Los cuatro recursos de seed comparten proxy; las dos vetas comparten
  silueta exterior y su núcleo queda poco visible desde una aproximación horizontal.
- **Colisión provisional (baja):** conserva los cubos originales invisibles para no alterar
  interacción/recorrido. No representa los huecos de la señal ni el contorno exacto de cada mineral.
- **Iluminación/rótulos (media):** capturas del juego muestran sombras muy oscuras y poco
  contraste del texto sobre fondo negro. Cielo, materiales del suelo e iluminación siguen
  pendientes; aprobar importación no equivale a aprobar presentación Q2.
- **Resuelto:** `E` sobre veta sin taladro mostraba el aviso correcto y lo sobrescribía
  inmediatamente con "Sin consola ARGOS...". Se preserva `tool_core_drill`; cubierto por
  smoke con input real sobre ambos tipos de veta.
- Pruebas gráficas automatizadas y capturas no reemplazan revisión humana ni medición de FPS.

## Deuda del pipeline de arte — 2026-09-05

- **Media, integración pendiente:** kit de cinco blockouts exportado y probado transitoriamente en Unreal, pero no guardado en Content ni conectado a gameplay. No hay habitación montada; no sustituye el bug de ESCOTILLA.
- **Media, colisión pendiente:** un convexo único del marco puede cerrar el paso. La prueba de importación desactiva colisión; próximo ensayo debe usar cajas por lateral/dintel y comprobar cápsula real.
- **Baja, calidad Q1:** UV0 debug superpuesto, colores planos; no lightmap/atlas de producción. Humano sin rig y consola de piezas rígidas, no certificados para destructibilidad ni skin. No se presentan como arte final.
- **Baja, entorno resuelta para esta corrida:** sandbox impidió iniciar Zen al validar Unreal; repetir fuera del sandbox con autorización permitió commandlet exitoso. `python` apunta al alias de Store sin intérprete: ejecutar manifiesto con Python incluido en Blender.
- **Seguimiento:** ver `ART_ASSET_MASTER_PLAN.md` para orden de producción y `BLENDER_PIPELINE.md` para regeneración. No hay ampliación de alcance a planetas completos, NPCs ni vuelo.

## Resuelto

### Interacción (`E`) sobre ESCOTILLA — RESUELTO (2026-09-05 Rebuild10, confirmado por el usuario en sesión manual)

- **Síntoma**: parado frente a `ESCOTILLA`, `E` no producía ninguna acción visible; `AGRSO`/consola sí respondía.
- **Causa raíz #1 (bloqueante real)**: las 12 rocas procedurales de `AAstraeonGameModeBase::MaterializeCurrentRegion` se generaban en un anillo de 3.5–14.5 m alrededor del origen, excluyendo sólo 250 cm alrededor del origen y 400 cm alrededor del pad de despliegue. La ESCOTILLA está a 620 cm del origen (fuera de esa exclusión) y ARGOS a ~286 cm (apenas afuera también), así que según la seed una roca podía terminar bloqueando físicamente el acceso a la ESCOTILLA. **Fix**: exclusión explícita de 300 cm alrededor de cada punto de interés de Ítaca (luego reforzado por Codex usando `AAstraeonItacaInterior::IsInsideFootprint`).
- **Causa raíz #2 (diagnóstico enmascarado)**: `AAstraeonGameModeBase::BeginPlay()` ejecuta `GEngine->Exec(..., "DisableAllScreenMessages", ...)`, así que cualquier `AddOnScreenDebugMessage` es un no-op silencioso — por eso la instrumentación inicial con ese canal no mostraba nada, ni para ARGOS ni para la ESCOTILLA. Se usó en su lugar una línea de debug dibujada directamente por `AAstraeonHUD` (removida una vez confirmado el fix).
- **Bug secundario encontrado y corregido en el camino**: variable local `Player` en `AAstraeonPlayerController::StartSelectedNewGame()` ocultaba el miembro `Player` de `APlayerController` (error de compilación C4458 con warnings-as-errors); renombrada a `SpawnedPlayerCharacter`.
- **Confirmación manual**: el usuario reportó recorrido completo exitoso — salió de Ítaca por la ESCOTILLA, recolectó recursos, fabricó el resonador y resolvió la señal.
- **Handoff técnico**: ver `Docs/HANDOFF_ESCOTILLA_INTERACCION.md` (histórico, ya no bloqueante).

### Vista mayormente negra al alejarse del punto de despliegue — fix incluido, falta confirmación visual

- **Síntoma**: caminando unos pocos pasos desde el spawn/hatch, la vista 3D pasaba a verse casi completamente negra; sólo el HUD era legible.
- **Estado**: fix incluido desde Rebuild4/Rebuild5: `AExponentialHeightFog` runtime, sol con tinte dorado-cálido, rocas procedurales ocre/naranja, colores en suelo y pad.
- **Impacto**: reducido por automatización, pero la calidad visual final depende de prueba humana.
- **Acción pendiente**: jugar el build y confirmar que alejarse del pad muestra bruma/terreno en vez de negro plano.

### Save/close/open/continue — RESUELTO (2026-09-05, confirmado por el usuario)

- **Prueba**: `F6` guardar → cerrar `Astraeon.exe` → reabrir → `F10` continuar.
- **Resultado**: el usuario confirmó que tanto guardar como continuar funcionan correctamente en sesión manual real.
- **Impacto**: cierra AC-11 para H6.

### Piso de Ítaca se superpone con la caja/paredes de `AAstraeonItacaInterior`

- **Síntoma reportado por el usuario (2026-09-05, sesión manual post Rebuild11)**: alrededor de Ítaca hay algo así como paredes (la nueva habitación de blockout de `AAstraeonItacaInterior`), y el piso de Ítaca parece "sobrepisarse" con algún otro objeto, aparentemente parte de esas paredes.
- **Diagnóstico**: `AAstraeonItacaInterior` (agregado en paralelo, ver `Source/Astraeon/Private/Environment/AstraeonItacaInterior.cpp`) tiene su propio `FloorCollision` (Z de -12 a 0) dentro del mismo footprint donde `AAstraeonGameModeBase::MaterializeCurrentRegion` sigue generando el cubo `Runtime_ProceduralRegionSurface` (Z de -100 a 0) — ambos "pisos" comparten la superficie superior en Z=0, lo que puede causar z-fighting/doble colisión.
- **Estado**: pendiente. No se tocó porque `AAstraeonItacaInterior` es una pieza de arte/nivel en desarrollo activo (aparentemente de otra sesión de trabajo en paralelo); antes de tocar la geometría conviene confirmar quién sigue esa pieza para no pisar cambios.
- **Acción recomendada**: cuando la habitación de Ítaca esté estable, decidir una única fuente de verdad para el piso interior (o recortar el cubo procedural de región para no solaparse con el footprint de `AAstraeonItacaInterior::IsInsideFootprint`).

### `F5`/`F9` chocaban con atajos de debug del motor — RESUELTO

- **Síntoma reportado por el usuario**: al presionar `F5` (Guardar) toda la pantalla cambiaba de color.
- **Causa**: `Engine/Config/BaseInput.ini` define `DebugExecBindings` propios del motor, activos en builds Development (no Shipping): `F1`-`F5` cambian el modo de visualización del viewport (wireframe/unlit/lit/shader complexity) y `F9` sale captura de pantalla (`shot showui`). Al coincidir con las teclas de proyecto `SaveGame`/`ContinueGame`, ambas acciones se disparaban a la vez.
- **Fix**: `SaveGame` movido a `F6` y `ContinueGame` a `F10` (libres de atajos de motor) en `Config/DefaultInput.ini`; hints del HUD actualizados.

### ESCOTILLA no es una puerta física — se puede salir de Ítaca caminando, sin `E`

- **Observación del usuario (2026-09-05)**: como la ESCOTILLA no tiene colisión propia (es sólo un marker interactuable), caminar a través de su posición ya te "saca" de la caja que rodea Ítaca sin necesidad de presionar `E`. `E` sólo dispara el teletransporte a la plataforma de despliegue en superficie (`GetSurfaceDeploymentLocationCm`), que es una acción distinta de "cruzar la puerta".
- **Estado**: backlog, no bloqueante. El usuario mismo lo marcó como "ahora o más adelante".
- **Acción recomendada a futuro**: si se quiere que la ESCOTILLA funcione como puerta real, agregarle colisión bloqueante que sólo se desactive momentáneamente al interactuar con `E` (o separar conceptualmente "cruzar el umbral" de "desplegar en superficie").

### Rendimiento en sesión jugable — RESUELTO (2026-09-05, confirmado por el usuario)

- **Prueba**: sesión manual jugable completa (post Rebuild12).
- **Resultado**: el usuario reportó rendimiento estable, sin tirones perceptibles.
- **Impacto**: cierra AC-14 para H6. No hay medición numérica de FPS exacta, sólo confirmación subjetiva de estabilidad; suficiente para este MVP temporal.

## Baja severidad

### NullRHI automation bootstrap logs `Condition failed`

- **Síntoma**: durante el arranque de `UnrealEditor-Cmd.exe -nullrhi`, antes de ejecutar la suite `Astraeon.*`, aparecen tres líneas:
  - `LogAutomationTest: Error: Condition failed`
- **Estado**: observado de forma recurrente.
- **Impacto**: bajo. La cola propia `Astraeon.*` termina exit code 0 y todos los tests del proyecto pasan.
- **Acción recomendada**: investigar sólo si empieza a afectar exit code, packaging o pruebas del proyecto.

### PowerShell no reporta exit code confiable para smoke packaged

- **Síntoma**: `Astraeon.exe -nullrhi ...` termina y no queda proceso vivo, pero `$LASTEXITCODE` aparece vacío en algunas corridas.
- **Estado**: observado en smoke simple y critical-path smoke packaged.
- **Impacto**: bajo-medio. La evidencia de log confirma inicialización/salida limpia y recorrido crítico exitoso.
- **Acción recomendada**: crear wrapper de smoke con timeout/proceso explícito o comando interno que escriba un sentinel file.

### Git Bash convierte rutas Unreal `/Game/...`

- **Síntoma**: al ejecutar `UnrealEditor-Cmd.exe ... /Game/Maps/L_AstraeonBootstrap` desde Git Bash, MSYS puede convertir el argumento a `C:/Program Files/Git/Game/Maps/L_AstraeonBootstrap`, causando fallo de carga y crash posterior.
- **Estado**: reproducido y evitado.
- **Impacto**: bajo si se usa PowerShell o `MSYS_NO_PATHCONV=1`.
- **Acción recomendada**: para comandos Unreal desde Git Bash con rutas `/Game/...`, prefijar `MSYS_NO_PATHCONV=1`.

### `CreateBootstrapMap.py` no regenera sobre mapa existente

- **Síntoma**: `EditorLevelLibrary.new_level` falla si `/Game/Maps/L_AstraeonBootstrap` ya existe; en UE 5.7 commandlet derivó en crash al intentar esa recreación.
- **Estado**: reproducido una vez durante una iteración previa; no afecta el flujo runtime ni el smoke de mapa existente.
- **Impacto**: bajo. Sólo afecta regenerar el `.umap` desde script.
- **Acción recomendada**: antes de usar el script como regenerador, reemplazarlo por una rutina explícita y verificada de cargar/limpiar o crear con autorización sobre el asset binario.

## Deuda técnica aceptada temporalmente

### HUD y menú C++ temporales

- **Motivo**: acelerar verificación mecánica del MVP antes de UI final.
- **Impacto**: presentación pobre, pero lógica testeable.
- **Cierre esperado**: reemplazar por UI Blueprint/UMG delgada cuando el flujo se estabilice.

### Marcadores con `TextRenderComponent`

- **Motivo**: diferenciar recursos, ARGOS, anomalía y señal sin assets externos.
- **Impacto**: útil para debug/jugabilidad temprana, no arte final.
- **Cierre esperado**: reemplazar por meshes/materiales/FX propios o licenciados.

### Ítaca/ARGOS incompleto visualmente

- **Motivo**: existe consola ARGOS funcional, pero no una estancia de nave presentable ni transición narrativa pulida.
- **Impacto**: el flujo cumple mecánicamente, no todavía como experiencia narrativa final.
- **Cierre esperado**: crear zona inicial de Ítaca, lectura clara de briefing y transición controlada a región.

### Cara del protagonista: sin parpadeo y boca que no abre

- **Motivo**: la cara tiene 249 vértices (arista 8,9 mm) y ~18 por ojo; la boca es geometría
  sellada, sin interior. `jaw_open` rota la mandíbula pero los labios no se separan.
- **Impacto**: nulo en juego hoy — es primera persona y con casco. Se notaría si alguna vez
  hay cámara sobre la cara.
- **Cierre esperado**: retopología densa sólo de la cabeza (~8–10k tris con bucles de ojo y
  boca), que habilitaría parpadeo y apertura real. Reabre bake y LOD.

### Silueta del pelo con muescas oscuras

- **Motivo**: pérdida de fidelidad de la retopología frente al arte original de 233k tris,
  que resuelve mechones que 65k no puede. Comparativa en
  `graphics/characters/main_player/references/QA_Compare_Source_Face.png`.
- **Impacto**: visible sólo en primer plano de la cabeza sin casco.
- **Cierre esperado**: mismo trabajo que el punto anterior.

### Casco: forma de placeholder

- **Motivo**: 970 triángulos de bandas rectas; el visor no envuelve y queda un hueco entre el
  borde inferior y el cuello. El sombreado suave y el set de textura ya están resueltos: lo
  que falta es diseño de forma.
- **Impacto**: se ve en cualquier plano del personaje con casco puesto.
- **Cierre esperado**: rediseño de la cúpula y el visor, acordado como trabajo futuro.

### 2026-09-08 — Resuelto: manos de primera persona fuera del encuadre

- **Motivo**: `SK_Human_HandsFP_Blockout` medía ±59,1 cm de ancho con su centro a 104,6 cm de
  altura: brazos colgando a los costados, no manos sostenidas frente a la cara.
- **Cierre**: `AN_HandsFP_{Idle,Walk,Run,Jump,Land}` copian la pose de brazos del agarre de
  escáner ya autorizado sobre los clips de locomoción, dejando torso y piernas intactos. La
  mano con el escáner aparece en el encuadre: `Docs/evidencia/QA_Personaje_PrimeraPersona_Integrado.png`.

### 2026-09-08 — Resuelto: el protagonista no se veía en juego

- **Motivo**: la importación FBX de animaciones sueltas perdía la escala de unidad del
  Armature (metros → centímetros). El esqueleto lleva `root` a escala 100 en su pose de
  referencia y las claves de animación se escribían a 1, así que al evaluar cualquier clip
  la pose colapsaba a 1/100: cabeza a **1,64 cm** de los pies. Por eso todos los indicadores
  de visibilidad daban correctos: la malla estaba, medía nada. Segunda causa concurrente:
  los materiales generados por script no declaraban `MATUSAGE_SKELETAL_MESH`.
- **Cierre**: `Scripts/Editor/CharacterAnimationScale.py` normaliza y valida la escala de
  `root`, aplicado a los 59 clips existentes y enganchado a todos los scripts de importación
  para que no vuelva a entrar. Verificado en editor y en el ejecutable empaquetado, con
  `HeadHeightCm=163.9` y capturas dentro de partida.
- **Detalle completo**: `Docs/INVESTIGACION_PERSONAJE_INVISIBLE.md`.
