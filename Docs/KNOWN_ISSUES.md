# Problemas conocidos — ASTRAEON

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
