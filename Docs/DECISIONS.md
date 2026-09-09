# Decisiones — ASTRAEON

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
