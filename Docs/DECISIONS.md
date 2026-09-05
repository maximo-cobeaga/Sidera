# Decisiones — ASTRAEON

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
