# Decisiones — ASTRAEON

## 2026-09-10 — P3.2: el relieve de la región viaja con la definición del cuerpo

La meseta bajo Ítaca, los claros del suelo y las exclusiones de montaña de Region A son parte del
relieve de Khepri. `FAstraeonPlanetDefinition` lleva un `FAstraeonPlanetRegionRelief` inmutable y
compartido, y `FAstraeonPlanetSurface::SampleRadialHeightCm` lo aplica. Así la malla, la colisión,
el validador de tránsito, `GetSurfacePointCm`, el HUD y los smokes leen la misma función sin que
nadie tenga que acordarse de pasar la región. La copia que recibe cada worker cuesta un contador.

- Fuera del cono de influencia de la región, la altura es la del cuerpo **bit a bit**: los mismos
  llamados sobre la misma entrada. `SurfaceContract` lo verifica sobre un patch del antípoda.
- La huella de Ítaca se mide en el marco propio de la nave (los ejes del plan llevados a su
  origen), el mismo con el que P3.3 la va a apoyar. Así la meseta queda justo debajo del casco.
- Las pruebas de distancia usan cosenos en vez de un `acos` por punto. Con eso el costo de los
  patches no se movió: 923 frames sobre 2 ms en la caminata de 90 s, contra 945 antes.
- La ubicación de Region A se resuelve una vez por proceso y queda en caché. La prueba de Khepri
  la vuelve a resolver desde cero para que el caché no esconda un no determinismo.

## 2026-09-10 — P3.1: Khepri mide 500 km; Region A tiene un lugar fijo

**Radio de Khepri: 500 km**, elegido por el propietario entre 50, 500 y 2.500 km. Es el tier Target
probado en la puerta de la Fase 2: el horizonte queda a ~1,3 km a la altura de los ojos y la
curvatura sólo se nota desde altura. El perfil del planeta gana radio y seed de cuerpo, y de ahí
sale su `FAstraeonPlanetDefinition`. La masa se deriva de la gravedad authored (8,05 m/s²) para
que no puedan contradecirse. El relieve pertenece al planeta: toda sesión camina el mismo Khepri.

**Region A es un lugar, no una seed.** Su diseño es fijo ("Region A fija", 2026-09-06), así que su
posición sobre Khepri también: la resuelve `FAstraeonPlanetTraversal` una vez, con una seed derivada
del id de la región, contra el plan de la región con Ítaca en la zona de aterrizaje. La seed de
contenido de la sesión no la mueve. El plan en metros se lleva a la esfera por mapa exponencial y
de vuelta; a 500 km la región de 500 m se desvía del plano tangente menos de 7 cm en su borde.

## 2026-09-10 — P2.7: tránsito validado sobre la esfera

El validador plano resolvía una sub-seed de relieve para que la región fuese transitable. En un
planeta el relieve es global: re-sembrarlo por región rompería a los vecinos. Lo que se resuelve
es dónde se ubica la región, con la misma forma: hasta 8 candidatos deterministas y una variante
segura documentada. La variante es la primera ancla cuya región entera no tiene capa de montaña;
el suelo solo respeta siempre el escalón (`Terrain.Relief`), así que es transitable por
construcción, y la prueba lo verifica igual. El plan de Region A en coordenadas planas se ubica
sobre el planeta por mapa exponencial hasta que la Fase 3 lo autoree sobre la esfera. El
validador plano sigue para la build plana.

## 2026-09-10 — P2.6: estado mutable, identidad de entidad y save v3

**Identidad de entidad.** Las criaturas planetarias se colocan por celdas de nivel fijo (~1 km),
con la seed del canal `Entities` derivada por hash estable de la celda. El id es cuerpo + celda +
índice; no depende del LOD ni del orden de carga. Un índice rechazado (montaña) no desplaza a los
demás.

**Estado.** `UAstraeonRuntimeStateManager` guarda deltas con dirección, altitud y celda. El
streaming nunca es dueño de estado: materializa la seed menos los deltas. La criatura abatida
lleva el mismo reloj de repoblado de 240 s que ya definía el diseño de fauna; "sigue abatida" es
hasta que vence, también a través de descargas y guardados.

**Hallazgo.** `RecordCreatureDeath` no tenía llamador: el reloj existía y se guardaba, pero ninguna
muerte lo arrancaba, así que la limitación de MV4 seguía viva también en el mundo plano. El disparo
ahora llama a `RecordCreatureDefeat`, que decide entre delta planetario y reloj de nido plano.

**Fauna en los laboratorios: apagada por defecto** (`bSpawnFauna`). Poblar un planeta es contenido
de la Fase 3, y un pastador persiguiendo al jugador contaminaría los bancos de locomoción. El
mecanismo de persistencia es de la Fase 2 y se prueba con `-AstraeonPlanetFauna`.

**Save v3.** La ubicación es cuerpo + dirección + altitud sobre el radio de referencia + rumbo
tangente; más los deltas. `PlayerTransform` e `ItacaOriginCm` quedan sólo para leer y migrar
archivos viejos. El mundo plano de v1/v2 vive sobre el plano tangente del ancla (0,0,1) de un
cuerpo documentado, `legacy_flat_region` de 10 km, por mapa exponencial: conserva distancias y
rumbos desde el ancla y se invierte exacto, así la build plana sigue cargando sus partidas. La
versión por defecto de la clase queda en 2 a propósito: la serialización puede omitir valores
iguales al defecto, y subirla haría leer un v2 como v3. Las estructuras colocadas del mundo plano
siguen en coordenadas planas: su proyección es contenido de la Fase 3.

## 2026-09-10 — P2.5: marco local por cambio de origen del mundo

`UAstraeonLocalFrameSubsystem` pide un nuevo origen de mundo cuando la cámara se aleja más de
5 km del actual; el motor lo aplica al principio del siguiente tick. Todo lo que guarda una
posición absoluta la desplaza en `ApplyWorldOffset` (centro del planeta en la gravedad, suelo
seguro y previsualización del personaje) o se guarda relativo al planeta (smokes).

Chaos no desplaza su escena de forma nativa en UE 5.7: el motor teletransporta cada cuerpo. En un
planeta son el anillo de colisión y algunos personajes; medido sin efecto en locomoción.

La medición A/B mostró que LWC ya da precisión suficiente a 2.500 km: con y sin marco local no
hay jitter. Se mantiene activo igual, porque acota las coordenadas absolutas cerca del jugador
para cualquier sistema que siga en float (audio, partículas, navegación) y no cuesta nada medible.
Se registra así, sin atribuirle una mejora que no se midió. `-AstraeonNoFrameShift` existe sólo
para repetir esa comparación.

## 2026-09-10 — P2.4: colisión por anillo de patches

El propietario autorizó cerrar la Fase 2 sin más pruebas humanas; la evidencia es automática y
visual (capturas revisadas por el agente).

La colisión se construye con el mismo constructor que los patches dibujados, al nivel más fino y
sin faldones, un componente por patch. Anillo de 40 m, conservación hasta 80 m. Los patches se
piden a una cola de workers propia para no competir con el render. Si el patch bajo el jugador
no está listo, se construye en el acto y se cuenta como emergencia: sólo debe pasar al arrancar
o tras un teletransporte. Un patch sobre el que alguien está parado nunca se retira. Cocinado
síncrono: el suelo existe en el instante del commit.

Se retiran el puente de P2.3, el doble búfer de Fase 1 y `-AstraeonPlanetLegacyFaces`, como
estaba acordado. `SpawnDirection` en el runtime decide dónde aparece una sesión nueva, y el
suelo se prepara antes del teletransporte.

## 2026-09-10 — Plan de cierre de P2.3 y gestor de patches

El propietario aprobó cerrar P2.3 en cuatro incrementos: A, gestor puro; B, backend
ProceduralMesh y runtime de `TL_11`; C, `TL_12_PatchLOD` y su smoke; D, documentación y prueba
humana. Tres decisiones suyas:

- **`TL_12` a 50 km de radio**, en vuelo por una ruta scripteada y sin colisión. Los 500 km de
  Target se prueban en P2.5, con marcos locales, para no mezclar LOD con precisión float.
- **Puente exacto de colisión en `TL_11`**: `FaceQuads = Quads << FinestAllowedLod`. Constructor,
  rejilla global entera y diagonales son los mismos, así que la colisión coincide triángulo por
  triángulo con los patches finos. Radio y cadencia del relevo fijos en metros. Sólo sirve en
  Lab; P2.4 lo reemplaza con el anillo.
- **`-AstraeonPlanetLegacyFaces`** conserva las seis caras fijas para comparar. Se borra al
  cerrar P2.4.

Diseño del gestor (A). El conjunto visible es siempre una partición de la esfera: un split
muestra los hijos cuando los cuatro están confirmados y un merge oculta los hijos cuando el
padre está confirmado. Mientras tanto, lo viejo sigue en pantalla. El delta de LOD puede superar
uno durante un relevo; por eso el selector separa `ValidatePartition` de `ValidateCover`. Un
build fallido no se sustituye por terreno plano: el patch que iba a reemplazar se queda. Las
solicitudes se ordenan por distancia al observador, con desempate determinista.

Integración (B). Las seis caras siguen construyéndose al empezar y se retiran en el frame en que
aparece la primera cobertura de patches: nunca hay un frame sin planeta y nunca se ven los dos
a la vez. Si el radio exige una rejilla de colisión mayor que 128 quads, el puente no puede ser
exacto; el runtime lo dice con un `Warning` y usa las seis caras, en vez de dejar suelo
desalineado en silencio. Selección cada 0,1 s y 2 subidas por frame: provisionales, se miden en
`TL_12`.

`TL_12` (C). El observador del LOD pasa a ser la cámara y no el pawn: el detalle sigue a lo que
se ve, y así un vuelo scripteado funciona sin inventar otro pawn. La velocidad para la
predicción se toma entre selecciones, no del pawn. `bNearCollision` apaga la colisión cercana
en laboratorios de LOD. El smoke falla sólo por invariantes duros —agujero o solape en pantalla,
build fallido, componentes sin cota o ruta que no asienta—; todo lo demás se mide primero y los
umbrales se ponen después.

El perfil de `TL_12` mostró un selector cuadrático. Se reemplaza por cola de prioridad y
balanceo incremental, sin cambiar un solo resultado: la clausura 2:1 de un quadtree es única y
se conserva el desempate. La versión anterior queda como `SelectReference` y una prueba compara
las dos en 624 vistas. No se lleva la selección a un worker todavía: con 1,1 ms de media ya no
es el cuello de botella, y moverla agrega latencia y otra cola.

Observador (D). Un mapa de laboratorio tiene que funcionar con Play, no sólo por línea de
comandos. En un planeta sin colisión cercana el personaje vuela en lugar de caer: es el mismo
personaje, con la misma cámara y la misma orientación radial, no un pawn aparte. Así el LOD se
inspecciona desde donde lo verá el jugador. No es la nave de la Fase 5 ni pretende serlo.

Desvío del plan: no se sube `MaxTrackedPatches` de 64 a 512. El gestor libera cada dirección
del streaming apenas recoge su resultado, así que ese registro cuenta trabajo pendiente —acotado
por los 2 trabajos simultáneos— y el conjunto confirmado es del gestor. La cola se abstrae en
`IAstraeonPlanetPatchBuildQueue` para probar el gestor con una cola determinista.

## 2026-09-10 — Selección quadtree LOD como contrato puro

La selección LOD se implementa como función de datos sin mundo ni UObjects. Comienza con una
hoja por cara, subdivide por error geométrico proyectado con predicción de velocidad y aplica
balanceo hasta que ningún vecino difiera en más de un nivel. Cada selección queda limitada a
384 hojas y se valida la cobertura completa de cada cara. El runtime de producción todavía no
la consume: la integración con workers, commit de malla y `TL_12_PatchLOD` permanece pendiente.

El error usado para decidir es una aproximación conservadora (sagita de la celda más un término
acotado de relieve); no se presenta como certificado geométrico. Los faldones siguen siendo la
solución inicial para el borde y se recalculan con el tamaño de la celda vecina. Stitching se
difiere hasta medir grietas visibles.

## 2026-09-10 — Primer incremento de Fase 2: identidad y generación de patches

La dirección de patch usa `BodyId` del contrato existente, cara, nivel y coordenadas enteras.
LOD 0 es una cara; cada nivel duplica la resolución por eje, hasta 24. La identidad de una
entidad persistente sigue siendo planetaria, independiente de esa dirección de representación.

`StableHash64` es FNV-1a sobre bytes explícitos, con formato propio 1: `ASTPATCH`, byte 1,
BodyId ASCII en minúsculas terminado en cero y ocho enteros LE32 (WorldSeed, BodySeed, canal,
cara, LOD, X, Y, versión de generador). Se rechazan identificadores fuera de letras ASCII,
dígitos, `_`, `-`, `.`. No se usan índices internos de FName ni memoria nativa de structs.
El vector fijo de referencia es `15f6d44ed0533175`; la prueba fija sus entradas.

La altura y las normales siguen consultándose por dirección global y BodySeed. **PatchSeed no
alimenta la altura**, porque eso rompería la coincidencia de dos patches vecinos. El relieve
sigue en versión 3: no se modifica la geografía aprobada en Fase 1.

Rejilla predeterminada 33×33, potencias de dos entre 4 y 128 quads. Los índices de superficie
se separan de los faldones radiales para que la colisión no incluya paredes artificiales.
El constructor mantiene el origen y la resta en double/cm. El faldón predeterminado de 100 cm
es una entrada provisional; su cobertura entre LODs debe medirse al integrar el selector.

`BuildFace` pasa a ser el adaptador del patch raíz sin faldones; `TL_11` consume el constructor
nuevo conservando sus seis caras y su relevo de colisión. El generador asíncrono se verifica
por separado antes de conectar el runtime al planificador LOD. Acepta dos trabajos pendientes
y hasta 64 direcciones registradas; devuelve `AtCapacity` explícito y requiere `Release` al
descargar. Son límites iniciales de la infraestructura, no un presupuesto de producción
aprobado. Las revisiones no se reinician al descargar; cada resultado debe seguir vigente
en el instante de commit. Los workers sólo capturan copias y un token de cancelación.

No se elige backend de producción ni se cierra la puerta de Fase 2 con esta evidencia.

## 2026-09-10 — Pose A y continuidad de locomoción

La captura del propietario mostró que la pose base de `Idle` y locomoción cerraba los brazos
contra el volumen de las hombreras. Se fija `upperarm X = -1,08 rad` en `Idle`, `Walk_*` y
`Run_*`: abre la silueta sin cambiar malla ni esqueleto. El ciclo `Walk_F` mantiene 37 frames,
30 FPS y 1,2 s; no se elimina el último frame porque el primer/último pose son coincidentes.
Para el runtime se incorpora histeresis en los umbrales Idle/Walk/Run y continuidad de fase
normalizada al cambiar entre ciclos. La mezcla artística completa con AnimBlueprint/BlendSpace
queda fuera de este parche y sigue como deuda de calidad posterior.

## 2026-09-10 — Variante aislada para animaciones pulidas

El lote pulido en Blender se importa a `/Game/Astraeon/Characters/Player/Optimized_Polished` y
el runtime lo consume desde esa ruta. La carpeta `/Optimized` se conserva como comparación y
rollback. La importación exige el esqueleto aprobado, normaliza la escala raíz en el editor,
guarda los paquetes sólo después de validar los 18 clips de gameplay y deja un reporte reproducible
en `ContentPipeline/reports/polished_character_animation_import.json`. Así la comparación visual
no depende de sobrescribir assets binarios existentes.

## 2026-09-09 — Continuación de Fase 1 y presupuesto de animaciones

El propietario autoriza continuar y dispone hasta 10 créditos para Higgsfield Bridge si fueran
necesarios al pulir animaciones después del núcleo planetario verificado. No es una orden de
gastar los créditos ni de generar otro personaje. Conservar skeleton, fuentes y clips aprobados.

Fase 1: cube-sphere de seis caras con rejilla uniforme, ruido de valor 3D continuo interpolado
en posición radial y colisión limitada a celdas próximas al observador. Esa colisión mínima
permite verificar marcha; quadtree, workers, revisión/cancelación y anillos LOD pertenecen a Fase 2.
Radio y alturas internas de esta primera API quedan en double y centímetros explícitos,
compatibles con el marco existente; gravedad en m/s² y masa en kg. No se fija el canon planetario.
Los tiers de ingeniería se verifican como datos; la vuelta manual usa radio de laboratorio de 200 m.
La elección actual de PMC es experimental, sin decisión sobre backend de producción.

## 2026-09-08 — Normalizar la escala de raíz en la importación, no en el runtime

La importación FBX de animaciones sueltas pierde la conversión metros→centímetros del
Armature de Blender: el esqueleto lleva `root` a escala 100 y las claves de animación quedan
a 1. Se corrige **en la importación** (`Scripts/Editor/CharacterAnimationScale.py`, enganchado
a todos los scripts de import y validación) y no compensando en C++ ni escalando componentes.
Razón: el defecto está en los datos, y cualquier compensación en runtime se multiplicaría con
la de un socket o un componente hijo. La normalización sólo actúa ante el desajuste exacto
1 contra 100 y aborta ante cualquier otro caso, para no "arreglar" animación legítima.
Se acompaña de `validate_pose_scale()`, que exige altura de cabeza entre 65 y 220 cm en cinco
muestras de cada clip, y de un smoke que pulsa la **tecla V real** y comprueba la altura de
cabeza medida en juego, en editor y sobre el ejecutable empaquetado.

## 2026-09-07 — Escena proxy aislada y generación pendiente

Con Bridge conectado, crear `CHR_Astraeon_Player_Work` preservando Scene, sus objetos
y fuentes Q1. Prefix PROXY y propiedad `PROXY_ONLY_NOT_GAME_READY` evitan promover
andamiaje a personaje final. Altura 1.83 m y 30 FPS confirmadas en instancia viva.
La solicitud Tripo detallada/PBR queda revisable en `GENERATION_REQUEST.json`, sin enviar:
el estimador obligatorio de la skill del Bridge no está expuesto. No sustituir el asset
esquelético por video. Continuar cuando se resuelva esa ruta o el modelado nativo.

## 2026-09-07 — Lote de protagonista independiente y QA explícito

El pedido explícito actual autoriza un personaje detallado sin convertir el blockout Q1
previo en arte final. Destino `graphics/characters/main_player/`; preservar generadores,
fuentes y paquetes anteriores. Contrato pendiente: altura 1,83 m, 30 FPS, Blender métrico,
FBX con escala 1 e importación aislada UE 5.7.4. No actualizar dependencias.
La API Higgsfield remota no demuestra acceso al Blender abierto. La reinspección expone
ahora el Bridge, pero el host Blender sigue desconectado. Construcción pendiente hasta
conectar el panel y comprobar acceso, como exige el pedido. No sustituirlo por un worker
remoto ni declarar una generación de video como animación esquelética validada.

## 2026-09-06 — Terreno procedural coherente con validación previa

Por corrección explícita del propietario, el terreno del MVP vuelve a generarse por seed. La
seed no autoriza resultados incoherentes: el generador debe garantizar plataforma de Ítaca,
salida, rutas, POIs y conectividad antes de materializar una región. La malla de cuenca A03 se
retira del runtime porque enterró la nave y el jugador; queda sólo como experimento técnico.
El contrato y orden de implementación están en `PROCEDURAL_TERRAIN_CONTRACT.md`.

## 2026-09-06 — Region A fija: Khepri, Cuenca de la Primera Señal

Para materializar la arquitectura de mundo fija sin crear más cuerpos, se definió un único
planeta MVP `planet_khepri` y su Region A `region_first_signal_basin`. Khepri combina gravedad
baja, atmósfera no respirable y desierto rocoso; la región mide 500 × 500 m y tiene landing
zone, señal, anomalía, nidos y recursos con coordenadas estables. Las seeds `100`–`500` quedan
reservadas para contenido secundario. El nombre y los datos son C++ textual para iteración y
tests; A02 decidirá cómo persistirlos y cargarlo sin romper saves existentes.

## 2026-09-06 — Seed de contenido separada de geografía y migración de saves

Las nuevas expediciones siempre inician en `planet_khepri` / `region_first_signal_basin`.
`ContentSeed` conserva la elección del jugador para población secundaria futura y se persiste
por separado de los IDs de perfil. Hasta A03, la topografía heredada queda fijada a la seed
ambiental 100: evita que una elección de variación cambie rutas, POIs o el punto de aterrizaje.
SaveGame v2 guarda ambos IDs y `ContentSeed`; cargar SaveGame v1 conserva datos y coordenadas,
los etiqueta `legacy_generated_*` y los vuelve a guardar como v2. La migración tiene prueba
automática.

## 2026-09-06 — Terreno propio sin compras, planificación previa

Por instrucción del propietario, presupuesto de assets cero. El próximo bloque será terreno
regional continuo, integración, rutas y materiales/rocas propios. Se documenta en
[PLAN_TERRENO_REGIONAL.md](PLAN_TERRENO_REGIONAL.md) y queda pendiente de ejecución.
La malla continua es la alternativa preferida a probar, no una arquitectura ya validada.
No se añaden dependencias ni se inicia implementación en esta entrega.

## 2026-09-06 — Primer lote humano con animaciones simples

Pedido del propietario: avanzar en el diseño Blender e incluir caminar y animaciones
básicas. Se amplía el primer lote de manos a una base corporal Q1 para revisar
locomoción sobre `SKEL_Humanoid_A`; no se produce todavía un catálogo modular de ropa.
Siete acciones en el sitio a 30 FPS; el Character conserva la responsabilidad del
desplazamiento y del salto. No cambiar controles, colisión ni gameplay para demostrar arte.

La fuente usa metros, frente -Y y raíz al suelo. FBX nombra temporalmente el objeto
contenedor `Armature`, siguiendo el tratamiento explícito del importador UE 5.7,
para conservar exactamente 57 huesos y evitar una raíz artificial. Skeleton propio,
sin promesa de retarget Manny/Quinn. Validación Unreal transitoria antes de conectar
el Character. Evidencias y deuda: `HUMANOID_ART.md`.

## 2026-09-06 — Siluetas regionales como bloque completo de presentación

- Elegido §3.4 del inventario actual: independiente del rig humano, mejora objetos ya
  jugables dentro del MVP. No amplía reglas de combate, vuelo ni generación planetaria.
- Siete meshes originales por bpy, un material plano cada uno, pivote inferior y dimensiones
  explícitas. Reutilizan validador estático y exportador FBX existentes.
- C++ mantiene proxy de interacción/colisión y transform persistente. Un componente hijo
  sin colisión presenta el arte con escala métrica absoluta; referencias desde el CDO
  incluyen assets en cook. Se conserva fallback para ids futuros.
- Los cuatro recursos de seed comparten proxy cristalino Q1, sin afirmar que resina,
  vidrio, sal y filamentos tengan la misma estructura científica. Diferenciarlos queda pendiente.
- Se conservan rótulos hasta diseñar UI. Q2 requiere revisión humana de lectura y colisión.
- Manifiesto complementario enlazado desde el catálogo principal; build aislada
  `WindowsRegionArt` conserva las entregas anteriores.
- Sin commit que mezcle cambios ajenos: esta integración depende de archivos modificados
  y no rastreados presentes al comenzar. Checkout limpio no validado en esta entrega.

## 2026-09-05 — Primer slice de arte: escala e Ítaca

**Decisión:** comenzar por un kit Q1 blockout con referencia humana de 1,80 m, suelo, pared, marco y consola; especificar familias futuras sin producirlas. Conservar cápsula de 1,92 m y cámara actual ~1,60 m. Marco con hueco de 1,30 × 2,20 m; grilla interior de 2 m. Dimensiones y paleta son propuestas reversibles de arte.

**Motivo:** el código ya tiene región acotada, marcadores y criatura proxy, pero no un interior de nave ni skeletons. El kit sirve al AC-03 sin introducir vuelo o planetas completos. La guía de planetas mantiene valor para preparación futura; no evidencia implementación runtime.

**Consecuencia:** herramientas y assets estáticos validados en Blender y mediante importación transitoria UE. Geometría generada se conserva localmente, fuentes/config/reportes son versionables. LFS añadido para futuras fuentes manuales `.blend`. Materiales de producción, colisión, mapa ensamblado, rig y prueba manual son pasos separados pendientes. `Docs/MODULAR_ASSET_GENERATION.md` se crea porque no existía. La entrega no modifica ni certifica parches ajenos de ESCOTILLA.

## 2026-09-05 — Normalizar documentos bajo `Docs/`

**Decisión:** copiar los documentos maestros existentes de la raíz a `Docs/` y mantener los originales por compatibilidad inicial.

**Motivo:** `AGENTS.md`, `README.md` y el prompt inicial declaran `Docs/` como ruta contractual, pero el repositorio inicial contenía los archivos en la raíz.

**Consecuencia:** las fuentes de verdad quedan disponibles en la ruta esperada sin eliminar archivos previos ni arriesgar pérdida de contexto.

## 2026-09-05 — Bootstrap manual de proyecto C++

**Decisión:** crear la estructura mínima de Unreal C++ directamente (`.uproject`, `Source/`, `Config/`) en lugar de depender de un template gráfico.

**Motivo:** es la alternativa más simple, verificable y controlada para iniciar H0 sin introducir contenido binario innecesario.

**Consecuencia:** falta generar un mapa `.umap` en una tarea posterior, pero el núcleo C++ puede compilarse y testearse temprano.

## 2026-09-05 — Ítaca funcional por marcadores y superficie runtime

**Decisión:** representar la estancia inicial mínima con marcadores C++ (`ARGOS`, `SURFACE HATCH`) y una superficie procedural runtime amplia, en vez de depender todavía de una escena binaria final.

**Motivo:** el MVP necesita una transición controlada a superficie y un recorrido manual caminable; mantenerlo en C++ conserva verificabilidad y evita assets externos.

**Consecuencia:** Ítaca sigue siendo presentación temporal, pero el flujo ya guía briefing → hatch → escaneo y la región generada tiene suelo transitable para smoke manual.

## 2026-09-05 — Iluminación y feedback runtime para smoke visual

**Decisión:** crear iluminación básica desde C++ en `BeginPlay`, desactivar mensajes on-screen de Unreal y mover el feedback de interacción/guardado al HUD propio.

**Motivo:** el primer smoke manual mostró pantalla mayormente negra, warning rojo de iluminación baked y texto acumulado en la esquina superior izquierda.

**Consecuencia:** el package deja de depender de iluminación prehorneada del mapa bootstrap y la presentación temporal queda más legible. La solución sigue siendo provisional hasta reemplazar HUD y escena por presentación final/UMG delgada.

## 2026-09-05 — Punto seguro de despliegue desde hatch

**Decisión:** centralizar el destino del `SURFACE HATCH` en `UAstraeonRegionMaterializer::GetSurfaceDeploymentLocationCm()` y materializar una plataforma runtime dedicada bajo ese punto.

**Motivo:** el smoke manual reportó caída del mapa al interactuar con el hatch; una coordenada hardcodeada en el personaje no garantizaba explícitamente una superficie caminable ni reseteaba movimiento.

**Consecuencia:** el teleport desde hatch queda verificable por automation/smoke, con movimiento detenido y modo walking restaurado. Sigue siendo una solución temporal hasta tener escena Ítaca y terreno final.

## 2026-09-05 — Endurecer despliegue de SURFACE HATCH y red de rescate anti-caída

**Decisión:** eliminar de `AAstraeonPlayerCharacter` el componente de colisión casero (`UBoxComponent` creado y registrado a mano) que se había añadido como parche para el hatch, y apoyarse en cambio en la plataforma de despliegue que `AAstraeonGameModeBase::MaterializeCurrentRegion()` ya crea con el mismo patrón probado (`AStaticMeshActor` + cubo + perfil `BlockAll`) que usan la superficie regional y el resto de la materialización runtime. Si el trace de piso en el destino del hatch no encuentra nada (por ejemplo, la región nunca se materializó en esa sesión), el personaje ahora le pide al GameMode `MaterializeCurrentRegion()` de nuevo antes de reintentar el trace, en vez de fabricar geometría de colisión ad-hoc. Además se añadió una red de rescate general: el personaje registra la última posición confirmada como "piso firme" (`IsMovingOnGround()`) y, si cae más de 2000 cm por debajo de ese punto sin haber vuelto a pisar suelo, se lo teletransporta de vuelta automáticamente.

**Motivo:** el reporte de usuario (`E` sobre `SURFACE HATCH` sigue tirando al jugador fuera del mapa) llegó después de que el fix anterior (coordinar el despliegue vía `GetSurfaceDeploymentLocationCm()` + `UBoxComponent` dinámico) sólo se había validado con un smoke automatizado que teletransporta al jugador y llama `Interact()` directamente, sin pasar por el aim/trace real ni por varios frames de juego reales; ese smoke no puede detectar problemas de timing de registro de componentes ni de colisión duplicada/solapada (existían simultáneamente la plataforma del GameMode, el `UBoxComponent` casero del personaje y la superficie regional, las tres casi en el mismo lugar). En lugar de seguir apilando parches puntuales sobre el mismo síntoma, se simplificó a una única fuente de verdad para el piso de despliegue (la del GameMode, ya usada y probada en el resto del mundo runtime) y se añadió una protección general independiente de la causa exacta.

**Consecuencia:** el código de despliegue queda más simple y reutiliza un patrón ya probado en vez de manipular un `UBoxComponent` a mano; cualquier caída fuera de una superficie válida en cualquier parte del juego (no sólo el hatch) ahora se corrige sola en un plazo corto en vez de dejar al jugador cayendo indefinidamente. Esta sesión no tiene acceso a Unreal Editor/UBT en la PC de MAXIMO (sólo edición de archivos), así que el cambio **no fue compilado ni probado todavía**: falta recompilar `AstraeonEditor`/`Astraeon`, correr la suite de Automation Tests y repetir el smoke manual (menú → ARGOS → SURFACE HATCH → escaneo → recursos → crafting → señal → F5/F9) antes de darlo por resuelto.

## 2026-09-05 — Mejora de diseño de escenario: niebla atmosférica y terreno procedural

**Decisión:** agregar `AExponentialHeightFog` runtime en `EnsureRuntimeLighting()`, un tinte cálido al sol direccional, y 12 rocas de terreno procedurales (cubo básico con escala/posición deterministas por world seed) en `MaterializeCurrentRegion()`.

**Motivo:** el smoke manual confirmó que alejarse unos pasos del punto de despliegue muestra el fondo de render vacío completamente negro, lo que hace muy difícil orientarse, apuntar a marcadores o tener sensación de "mundo". La causa raíz es que la materialización sólo crea una plataforma acotada y la SkyLight captura una escena vacía. Sin atlas ni assets externos, la solución más efectiva es `ExponentialHeightFog` (rellena el horizonte con bruma sin requerir geometría de cielo) más terreno disperso basado en la seed para dar volumen visual alrededor del área de juego.

**Consecuencia:** el sol pasa a tener tinte cálido dorado/naranja (alien distant star). La niebla comienza a los 8 m con densidad 0.04 y caída suave (falloff 0.15), lo que deja los marcadores a distancia de juego (~10–20 m) claramente visibles pero llena el horizonte con bruma en vez de negro. Las 12 rocas procedurales (3.5–14.5 m de radio, 0.25–1.55 m de alto, 0.6–3.0 m de huella) son colisionables, se excluyen de 2.5 m alrededor del origen y 4 m alrededor del deployment pad, y se etiquetan `AstraeonRuntimeSurface` para re-materializarse limpiamente. Las rocas son deterministas por seed: la misma seed siempre produce el mismo layout de terreno. El código **no compilado todavía** — falta recompilar, repackagear y verificar manualmente.

## 2026-09-05 — Confirmación manual del fix anti-caída y mira/feedback de interacción

**Decisión:** dar por confirmado (vía prueba manual real del usuario sobre el package recompilado) el endurecimiento de despliegue de `SURFACE HATCH` del punto anterior ("piso único + red de rescate"), y agregar al HUD una mira (crosshair) en el centro de pantalla más un color distintivo para la línea `Feedback:`.

**Motivo:** el usuario confirmó jugando el build recompilado que ya no se cae del mapa al usar el hatch. Sin embargo reportó dos observaciones nuevas: (1) no percibe que pase nada visible al presionar `E` sobre `SURFACE HATCH`, y (2) al escanear un recurso ("el supuesto árbol") lo identifica correctamente pero `E` no lo recolecta. Se investigó el código de `AAstraeonPlayerCharacter::Interact()`, `DeployToSurface()`, `UAstraeonGameInstance::AddInventoryItem()` y `RecordSurfaceDeployment()`: los tres casos de éxito (hatch, recurso, briefing ARGOS) sí actualizan `SetLastFeedbackMessage(...)` y la bitácora correctamente — no hay ningún camino de "éxito silencioso" en ese código. En cambio, se confirmó por inspección de `AAstraeonHUD::DrawHUD()` que **no existe ningún crosshair/retícula** dibujado en pantalla, pese a que tanto `Interact` (`E`) como `Scan` (click izquierdo) disparan un trace desde la cámara a lo largo de `GetControlRotation().Vector()` (es decir, el centro exacto de la pantalla en primera persona). Sin ninguna referencia visual de hacia dónde apunta la cámara, y con el único indicio de éxito siendo una línea de texto más entre otras doce en la esquina superior izquierda (mismo color blanco que el resto), es muy fácil: (a) no estar apuntando realmente al hatch/recurso cuando se presiona `E` (trace falla en silencio, sin ningún mensaje de error visible porque el jugador no lo relaciona con esa línea de texto), y (b) no notar el cambio de la línea `Feedback:` incluso cuando la interacción sí tuvo éxito. Se reprodujo parcialmente por control remoto: al caminar unos pasos desde el punto de spawn la vista pasa a verse casi completamente negra (no hay `SkyLight`/atmósfera real más allá de una luz direccional e intensidad de skylight bajas capturando una escena vacía, y el mundo materializado es sólo una plataforma acotada), lo que hace aún más difícil ver un marcador pequeño para apuntarle con precisión sin retícula.

**Consecuencia:** se agregó una mira simple (cruz de 4 líneas) dibujada siempre en el centro exacto del canvas del HUD (excepto en el menú), y la línea `Feedback:` ahora se dibuja en un verde distintivo en vez de blanco plano, para que un cambio de estado tras `E`/click sea mucho más difícil de pasar por alto. Esto no cambia ninguna lógica de gameplay, sólo agrega una referencia visual de apuntado y mejora la legibilidad del HUD existente — cambio de bajo riesgo, sólo dibujo 2D en `AAstraeonHUD::DrawHUD()`. Queda como deuda técnica separada (no abordada en esta iteración por requerir más que un cambio de código verificable a ciegas) la falta de iluminación ambiental/atmósfera fuera de la plataforma materializada, que hace que alejarse del punto de despliegue muestre una vista mayormente negra; se documenta en `KNOWN_ISSUES.md`. Este cambio **no fue compilado ni probado todavía** en esta iteración — falta recompilar, repackagear y repetir el smoke manual apuntando deliberadamente con la nueva mira al hatch y a un recurso para confirmar que ambas interacciones sí registran (y que el jugador ahora lo nota).

## 2026-09-05 — Interacción MVP por proximidad sobre marcadores

**Decisión:** mantener el line trace directo como interacción preferida, pero agregar un fallback por proximidad de 180 cm que selecciona el marcador runtime más cercano cuando el trace no impacta un `AAstraeonRegionMarker`.

**Motivo:** el jugador podía estar al lado de la ESCOTILLA o de un recurso y aun así fallar `E` porque los marcadores temporales son cubos pequeños y el trace exige apuntado preciso. Para un vertical slice jugable, la interacción obligatoria no debe depender de precisión milimétrica.

**Consecuencia:** `E` ahora funciona de forma tolerante sobre ARGOS, ESCOTILLA, recursos y SEÑAL cuando el jugador está cerca. El smoke crítico automatizado reproduce el fallo manual —apunta horizontalmente por encima del cubo bajo de ESCOTILLA— y pasa gracias al fallback. Sigue siendo una solución temporal hasta reemplazar marcadores por meshes/UX finales.

## 2026-09-05 — Sincronizar pruebas con UI española temporal

**Decisión:** actualizar expectativas de Automation Tests para las cadenas actuales de UI/HUD en español (`Semilla`, `ESCOTILLA`, `SEÑAL`, `Pista`, `Estado`, etc.).

**Motivo:** otro agente había cambiado/normalizado la presentación del HUD a español, pero varias pruebas seguían esperando textos en inglés. Eso rompía la suite sin indicar un bug de gameplay.

**Consecuencia:** las pruebas vuelven a validar el contrato visible real del build actual. Si más adelante se decide localizar formalmente el juego, hay que separar IDs/semántica de tests de las cadenas localizadas.

## 2026-09-05 — ESCOTILLA con interacción amplia y blanco temporal más alto

**Decisión:** ampliar el fallback de interacción de marcadores a 450 cm, elegir primero el marcador más cercano a la mira dentro de una tolerancia de 160 cm, conservar fallback de marcador muy cercano a 220 cm, y aumentar la escala Z temporal de la ESCOTILLA de `0.35` a `1.2`.

**Motivo:** el usuario confirmó que la ESCOTILLA seguía sin funcionar en juego real. El fix anterior de 180 cm pasaba el smoke, pero era insuficiente para una posición manual real y para un cubo demasiado bajo. Además se detectó que un intento de repackage no pudo sobrescribir el ejecutable porque el juego estaba abierto, lo que podía dejar al usuario probando un build viejo.

**Consecuencia:** la interacción obligatoria ya no depende de apuntado perfecto ni de estar exactamente pegado al centro del cubo. El smoke crítico ahora se ejecuta desde más lejos y mirando horizontalmente por encima de la ESCOTILLA, y el packaged smoke pasa después de cerrar el ejecutable viejo y reempaquetar correctamente.

## 2026-09-05 — Detener parches a ciegas sobre ESCOTILLA y dejar handoff

**Decisión:** no seguir ampliando tolerancias ni reestructurando input sin diagnóstico. Documentar el bloqueo manual en `Docs/HANDOFF_ESCOTILLA_INTERACCION.md` y dejar que el próximo agente lo aborde con instrumentación visible primero.

**Motivo:** el usuario confirmó que, aun parado frente a la ESCOTILLA en Rebuild6, `E` no produce ninguna acción visible. El smoke automático llama `Interact()` directamente y puede pasar aunque el input manual, el marker runtime o la sesión real fallen.

**Consecuencia:** el próximo agente debe diferenciar input no recibido, marker no encontrado y condición de despliegue fallida antes de implementar el fix definitivo.

## 2026-09-08 — Convivir con dos esqueletos en el personaje

**Decisión:** el cuerpo de sombra usa el esqueleto del protagonista (75 huesos) y las manos de
primera persona siguen sobre `SKEL_Humanoid_A` (57). No se unifican.

**Motivo:** medido sobre el rig, el eje que lleva el brazo al frente es **Z**, y los 45 clips
existentes lo usan entre 0,13 y 0,46 — casi nada. En `TwoHand_Idle` las manos quedan a
x = ±0,48 m del cuerpo, e `Inspect` sube la mano al pecho igual de abierta. Migrar el rig de
manos al esqueleto nuevo habría exigido re-autorizar las poses de brazo de los 45 clips para
que las manos cayeran delante de la cámara: una reescritura de la animación, con riesgo de
regresar clips ya auditados y sin beneficio adicional para el jugador. Separar brazos de
primera persona y cuerpo de tercera es además la arquitectura habitual del género, y aquí el
cuerpo sólo existe para proyectar la silueta.

**Consecuencia:** `Astraeon.Art.Character.FirstPersonRigIsWired` dejó de exigir que ambos
compartan esqueleto. Esa igualdad no era el requisito: era una comprobación indirecta. Se
sustituyó por las directas —cada gesto resuelve sobre el esqueleto de las manos, el cuerpo
está skinneado a un esqueleto con raíz `root`, la silueta mide entre 170 y 195 cm y la raíz
se apoya en el suelo de la cápsula—. Si algún día hay cámara en tercera persona sobre el
personaje, habrá que revisar esta decisión.

## 2026-09-08 — Tres morph targets faciales en lugar de siete

**Decisión:** autorizar `jaw_open`, `brow_raise` y `brow_furrow`, y bajar de 7 a 3 el umbral
que `Scripts/Editor/MainCharacterAppearance.py` exige al importar.

**Motivo:** la cara tiene 249 vértices con arista media de 8,9 mm y ~18 por ojo. Un parpadeo
necesita bucles de párpado que esa topología no tiene; con esa densidad el resultado sería
peor que no tenerlo. El umbral de 7 lo había escrito una sesión anterior por adelantado, sin
llegar a ejecutarlo nunca, y no era alcanzable sin rehacer la cabeza.

**Consecuencia:** queda registrado como limitación conocida que `jaw_open` baja la mandíbula
pero no separa los labios —la boca es geometría sellada, sin interior— y que no hay parpadeo.
La retopología densa de cabeza queda como trabajo futuro acordado.
