# Estado de desarrollo — ASTRAEON

## 2026-09-07 — Bridge conectado; protagonista en proxy de proporciones

- Conexión e inspección de Blender 5.2.1 LTS verificadas; test reversible de geometría
  y keyframes PASS. El bloqueo de conexión de la sección anterior quedó resuelto.
- Escena aislada `CHR_Astraeon_Player_Work`, 23 mallas proxy, cámaras frontal/perfil, 30 FPS,
  altura medida 1.83 m. Suelas corregidas y normales/escala/shells comprobadas.
  *(El `.blend` de este proxy quedó superado y se eliminó; los 23 objetos `PROXY_*` siguen
  ocultos dentro de `blender/CHR_Astraeon_Player.blend` — ver la sección del 2026-09-07 más abajo.)*
- Proxy no riggeado: rostro, traje final, animación, UV/PBR y export Unreal pendientes.
- Solicitud Tripo preparada, sin enviar; Bridge carece de `bl_estimate_generation`.
  Excepción para una generación con costo no estimable o modelado nativo pendiente de respuesta.
- Evidencia actual: `graphics/characters/main_player/docs/bridge_proxy_validation_20260907.json`.
  Detalle y ruta de reanudación en `MAIN_CHARACTER_PIPELINE.md` del mismo directorio.

## 2026-09-07 — Personaje principal: preflight, producción bloqueada

- Pedido nuevo de personaje masculino completo mediante Higgsfield Bridge: aún no construido.
- Reinspección vigente: herramientas `mcp__higgsfield_bridge__bl_*` disponibles; el Bridge
  responde, pero `get_host_status` informa `blr:false` y la consulta de escena falla con
  `Blender is not connected`. Se abrió Blender con autorización (PID 11784, responde),
  pero sigue sin conectar su panel. El 401 previo es histórico.
- Prueba aislada Blender 5.2.1 LTS sobre fuente humana previa: crear/modificar/retirar
  malla y crear keyframes PASS; archivo fuente intacto. No equivale a QA del nuevo personaje.
- Informe y contrato pendiente: [MAIN_CHARACTER_PIPELINE.md](../graphics/characters/main_player/docs/MAIN_CHARACTER_PIPELINE.md).
- Siguiente paso de este lote: en Blender abierto, panel Higgsfield → Supercomputer Connection
  → Connect; comprobar acceso a escena local y realizar prueba reversible allí.
  No hay nuevos `.blend`, FBX, texturas, paquetes Unreal ni cambios de gameplay.

## Arquitectura de mundo vigente — regiones fijas, 2026-09-06

- Directiva del propietario registrada en [MVP_WORLD_ARCHITECTURE.md](MVP_WORLD_ARCHITECTURE.md):
  mundo fijo → regiones diseñadas → POIs principales estables → variación local determinista.
- Auditoría en [WORLD_ARCHITECTURE_AUDIT.md](WORLD_ARCHITECTURE_AUDIT.md): no existen aún
  PlanetProfile, PCG, World Partition ni generadores planetarios; los generadores actuales
  se adaptan, no se eliminan. ADR 0003 deja la decisión aceptada.
- A01 completado: `planet_khepri` y `region_first_signal_basin` son perfiles C++ con ambiente,
  landing zone, recursos y POIs fijos. El diseño de dos rutas y exclusiones está en
  `DISENO_RECORRIDO_REGIONAL.md`; Automation 54/54 pasa.
- A02 completado: las partidas nuevas cargan Khepri/Region A, con recursos y POIs principales
  fijos. La seed de menú sólo identifica variación secundaria; la topografía temporal usa la
  seed ambiental fija 100 hasta A03. SaveGame v2 persiste IDs de perfil y ContentSeed; una
  partida v1 mantiene su layout y progreso, se etiqueta como legado y se regraba como v2.
  Build editor, 55 Automation Tests y smoke crítico pasaron.
- Próximo trabajo: A03, especificar e implementar la superficie diseñada de Region A antes de
  decidir la adopción del prototipo de malla continua.

## Plan vigente — terreno regional propio, 2026-09-06

- El propietario descartó compras por falta de presupuesto y pidió documentar antes de avanzar.
- Plan de ejecución: [PLAN_TERRENO_REGIONAL.md](PLAN_TERRENO_REGIONAL.md), tareas T00–T06
  registradas como pendientes en el backlog. Incluye qué, cómo, dónde, responsables y aceptación.
- T00 terminado: `FAstraeonTerrainSurfaceContext` y `SampleSurface` fijan una consulta pura
  de altura, normal y validez que incluye claros, exclusiones y plataforma de Ítaca.
- T01 en verificación: `AAstraeonTerrainSurfacePrototype` genera una malla continua aislada
  de 25.921 vértices / 51.200 triángulos, con colisión solicitada. El campo de cubos sigue
  siendo el terreno jugable hasta medir colisión runtime, memoria y rendimiento.
- `AstraeonEditor Win64 Development` compiló correctamente; Automation 53/53 pasó fuera del
  sandbox. El sandbox sin caché Zen/DDC falló antes de iniciar la cola; evidencia y solución
  en `TEST_REPORT.md`. Próximo paso: spawn/runtime trace del prototipo, no T02.

## Actualización vigente — exterior de Ítaca y estaciones, 2026-09-06

- La nave deja de ser un cubo gris escalado: casco de 13 m con morro rasante, dos góndolas,
  cuatro patines y antena, montados como módulos separados en `AAstraeonShipPawn`. Conjunto
  verificado contra el volumen de estudio del plan maestro: **13,0 × 10,0 × 5,87 m**.
- `itaca_pilot_console` e `itaca_fabricator` dejan de ser cubos: malla propia apoyada en el
  suelo y caja de colisión con su silueta, igual que ARGOS.
- **La ESCOTILLA es una puerta de verdad**: hoja sobre bisagra, cerrada al empezar, que
  bloquea el paso y se abre 95° hacia dentro al usarla con `E`. Cierra la entrada de
  `KNOWN_ISSUES.md` sobre "puerta siempre abierta". Su caja ignora el canal de visibilidad
  para no volver inalcanzable al propio marcador.
- 7 paquetes nuevos en `Content/Astraeon/Art/Blockouts/Itaca/`, reutilizando los materiales
  del kit. Compilación limpia; **Automation 51/51**; smoke crítico verde atravesando una
  escotilla que ahora empieza cerrada.
- **Falta revisión humana** de las dos previews y de la sesión jugable. Detalles y límites en
  `ITACA_EXTERIOR_INTEGRATION.md`.
- Próximo bloque de arte: terreno, rocas y cielo; después UMG, VFX y audio, que siguen sin
  existir. Motores y patines ya tienen dónde colgar los VFX de empuje y polvo.


## Actualización vigente — criatura con rig y estados, 2026-09-06

- `Umbra Grazer` deja de ser una esfera escalada: `SKEL_Quadruped_A` de 37 huesos, malla base
  y variante plateada (AC-06) de ~5.100 tris, y siete clips, uno por estado real del código
  (pastar, caminar, alerta, amenaza, huida, impacto y muerte).
- 14 paquetes nuevos en `Content/Astraeon/Art/Blockouts/Creatures/`. Colisión, alcance de
  disparo, escaneo y daño por contacto **sin cambios**: el envolvente de 1,4 × 0,8 × 0,7 m
  sigue ahí, ahora invisible.
- Patrullar alterna pastar y caminar; matar deja cadáver visible durante la caída en vez de
  destruir el actor en el frame del disparo. Se eliminó la escala como señal de estado.
- Compilación limpia; **Automation 50/50**; smoke crítico editor-game verde.
- **Falta revisión humana de las previews**, una por clip. El gesto de amenaza se distingue
  del reposo pero es sutil. Detalles y límites en `CREATURE_ART_INTEGRATION.md`.
- Próximo bloque de arte: exterior de Ítaca (casco, motores, tren, hoja de escotilla
  articulada y puesto de pilotaje), hoy un cubo gris escalado en `AAstraeonShipPawn`.


## Actualización vigente — personaje y herramientas integrados, 2026-09-06

- Lote de herramientas desbloqueado: `tools_unreal_import.json` estaba en `passed:false` por
  un chequeo que no aplicaba el espejo en Y de `convert_scene`, no por el arte. 8 mallas y
  7 animaciones aprobadas; manifiesto de 15 exports generado.
- 35 paquetes nuevos en `Content/`: `Human/` (SKEL_Humanoid_A, cuerpo, manos FP, 7 clips,
  4 materiales) y `Tools/` (7 herramientas + broca, 7 clips de uso, 6 materiales).
- `UAstraeonFirstPersonRigComponent` monta manos FP sobre la cámara y la herramienta sobre
  `socket_tool_r`; el cuerpo completo queda como sombra propia. Selección de clip en C++,
  sin AnimBlueprint. Ningún `FName` de ítem, receta o marcador cambió.
- Compilación limpia; **Automation 49/49**; smoke crítico editor-game verde
  (`Deployed/Scanned/Crafted/Resolved/Saved/Loaded/LoadedResolved=true`, seed 13579).
- Corregidos dos tests obsoletos previos a este trabajo (`WorldGen.Region.Invariants` y
  `Art.Region.PresentationPreservesGameplay`), rotos por el paso a 5 puntos de aparición de
  criatura de la fase de mundo vivo. Sin cambios de gameplay.
- **Falta revisión humana del encuadre**: offset de manos, pose de agarre y eje de la broca
  están derivados geométricamente, no confirmados en cámara. Detalles, comandos y límites en
  `CHARACTER_TOOLS_INTEGRATION.md`.
- Próximo bloque de arte: criatura `Umbra Grazer` (modelo, rig y cinco animaciones de
  estado), que hoy sigue siendo una esfera escalada y sostiene AC-06.


## Actualización vigente — personaje Blender, 2026-09-06

- Primer lote humano Q1: esqueleto de 57 huesos, cuerpo vestido de 1,80 m y manos FP.
- Siete animaciones simples: reposo, caminar, correr, salto, aterrizaje, interacción y
  agarre. Fuentes `.blend`, nueve FBX y previews en `ContentPipeline/Generated/HumanoidBlockout/`.
- Blender: validación de skin, cuatro controles negativos, determinismo, 261 fotogramas
  y nueve roundtrips aprobados. Unreal: dos mallas y siete animaciones importadas en memoria,
  escala/jerarquía/movimiento verificados; exit 0, commandlet 0 errores/0 warnings.
- **Aún no conectado al Character ni guardado como paquetes Unreal.** La build jugable
  anterior no incorpora cuerpo/manos/animación. No hubo cambios C++ ni nuevo empaquetado.
- Próximo lote: herramientas con agarres; montaje FP y animación runtime requieren
  integración y smoke posterior. Detalles, archivos y límites en `HUMANOID_ART.md`.
- Los avisos siguientes sobre ausencia de skeletons/animación describen el estado previo
  de producción o el runtime; ahora sí existen fuentes animadas Blender verificadas.

## Actualización vigente — arte regional, 2026-09-06

- Siete blockouts originales integrados para recursos, vetas, señal y anomalía; generación
  Blender reproducible, siete importaciones Unreal verificadas y manifiesto complementario.
- `AstraeonEditor` compila; suite completa **46/46** aprobada. Smoke renderizado revisado en
  siete capturas 1920×1080: escaneo real, recolección de básicos y rechazo de vetas sin taladro.
- Corregido el feedback que ocultaba el requisito del taladro. Smoke crítico editor-game
  con despliegue, escaneo, crafting, resolución y save/load aprobado.
- Build Development generada en `Builds/WindowsRegionArt`; pruebas del ejecutable y detalles
  en `REGION_ART_INTEGRATION.md` y `TEST_REPORT.md`.
- Próximo bloque: `SKEL_Humanoid_A` y manos FP, con deformación/importación probadas antes de
  animar herramientas. Todavía no existen esqueletos, animaciones, audio, VFX ni UMG.
- Los avisos de 2026-09-05 sobre vuelo sin compilar y ESCOTILLA bloqueada son históricos y
  quedaron superados por los handoffs posteriores. Esta entrega no certifica arte final
  ni el cierre global de todos los criterios MVP.

> **2026-09-05 — Fase actual: mundo vivo (post-MVP).** El recorrido crítico quedó validado
> a mano y el proyecto cruzó la puerta de salida del MVP. El trabajo en curso es *Ítaca como
> nave pilotable*. **Hay código de vuelo escrito y sin compilar/verificar**: antes de tocar
> nada, leer `Docs/HANDOFF_FASE_MUNDO_VIVO.md` §5 ("Estado de verificación").

## Entrega de arte 2026-09-05 — pipeline inicial verificado

- Plan de familias/prioridades en `ART_ASSET_MASTER_PLAN.md`, dirección visual y contrato modular documentados. Se respeta el MVP; planetas y seis biomas quedan planificados para después.
- Primer kit original: referencia humana 1,80 m, suelo, pared, marco de escotilla con hueco real y consola ARGOS. Son blockouts estáticos, no estancia montada ni personaje animado.
- Generación reproducible mediante `Tools/Blender/`, configuración textual, FBX por asset, fuente `.blend` y preview en `ContentPipeline/Generated/ItacaBlockout/` (outputs locales ignorados).
- Blender 5.2.1 LTS: 15 controles de prueba pasaron (10 negativos y 5 repeticiones), 5 assets validados y 5 roundtrips FBX con escala correcta. Preview revisada visualmente.
- Unreal 5.7.4: 5 importaciones transitorias válidas, dimensiones correctas en cm, exit 0, commandlet 0 errores/0 warnings. No se guardaron paquetes ni se modificó el mapa jugable.
- Manifiesto y reportes en `ContentPipeline/`; integración, materiales de producción, rig y colisión siguen pendientes. Detalles/comandos en `BLENDER_PIPELINE.md` y `TEST_REPORT.md`.
- Próximo bloque de arte: montar el kit en prueba aislada con colisión de marco que respete el hueco, comprobar gálibo de cápsula y vista a 1,60 m; integrar estancia después de diagnosticar el bloqueo de ESCOTILLA. No reclamar que la prueba de importación resuelve el input manual.
- Se conservaron cambios de gameplay/documentación ya presentes. No se recompiló C++ ni se reempaquetó: esta entrega afecta herramientas offline y no cierra un hito jugable.

## Resumen actual

- Fase: H1-H6 parcial — vertical slice mecánico temporal con package Development y recorrido crítico automatizado.
- H0: completo.
- H1: funcionalmente avanzado; menú C++ mínimo, seed editable, iniciar/continuar y guardado con F6 presentes. Falta validación manual completa.
- H2: funcionalmente avanzado; mundo regional determinista, supervivencia y mapa revelable existen. Falta balance/UX.
- H3: funcionalmente avanzado; mob con patrulla, estados, escaneo y daño por amenaza. Falta pulido audiovisual.
- H4: funcionalmente avanzado; recolección, inventario, crafting, barrera de señal y marcadores legibles implementados. Falta UX final.
- H5: avanzado mecánicamente; ARGOS, hatch de despliegue, hints, fuente de señal y completitud existen. Falta escena Ítaca visual final y ritmo narrativo.
- H6: en progreso; tests, package Development, smoke de mapa, smoke crítico editor-game y smoke crítico packaged pasan. **ESCOTILLA confirmada funcionando en smoke manual real (2026-09-05, Rebuild10/11)**: el usuario completó el recorrido crítico completo (salir por ESCOTILLA, recolectar, fabricar, resolver señal) en sesión jugable. Ver `Docs/KNOWN_ISSUES.md` para causa raíz y fix.

## Capacidades implementadas

- Proyecto Unreal C++ `Astraeon` para Unreal Engine 5.7.
- GameMode C++ con:
  - materialización runtime de región cuando se inicia o continúa partida;
  - modo smoke automatizado `-AstraeonAutoSmokeCriticalPath` para validar recorrido completo y save/load desde runtime;
  - smoke de hatch que apunta por encima del cubo bajo y confirma interacción por proximidad.
- PlayerController C++ con menú inicial:
  - `PageUp/PageDown`: cambiar seed seleccionada;
  - `Enter`: iniciar nueva partida;
  - `F10`: continuar save default;
  - `F6`: guardar partida en sesión activa;
  - `-AstraeonSeed=<n>`: seed inicial desde línea de comando.
- Character primera persona básico con movimiento, salto, correr (`Shift`, 520→900 cm/s), escaneo, interacción tolerante por trace + proximidad, crafting temporal y rescate anti-caída.
- HUD C++ temporal con panel sombreado, menú, seed, ambiente, región, mapa revelado, inventario, objetivo, hint accionable, feedback de acciones, vista de bitácora completa (`L` para abrir/cerrar, título+resumen por entrada), estado del traje y mira central.
- Generador ambiental determinista con gravedad, temperatura, presión, atmósfera simplificada, respirabilidad y riesgo.
- Generador regional determinista con tres recursos, fuente de señal, origen de mob y anomalía menor.
- Materialización runtime de superficie regional caminable, plataforma segura de despliegue, iluminación runtime básica, bruma atmosférica, rocas procedurales, marcadores de recursos/POIs, consola ARGOS temporal y hatch de despliegue a superficie.
- Marcadores con labels/colores temporales para distinguir `ARGOS`, `ESCOTILLA`, `SEÑAL`, `ANOMALÍA` y recursos.
- Mapa revelable por celdas, persistido.
- Traje con oxígeno, salud, daño ambiental simple, daño directo por amenaza y modificador de movilidad por gravedad.
- Mob `Umbra Grazer` con patrulla circular visible, estados por distancia, escala visual por estado y daño al traje en radio de amenaza.
- Escaneo de ambiente y criatura con entradas de bitácora.
- Inventario mínimo y recolección con `E`.
- Crafting de `signal_resonator` con `C` usando `silicate_fiber`, `ferrite_nodule` y recurso característico de la seed.
- Consola `itaca_argos_console` que registra briefing de ARGOS en bitácora.
- Hatch `itaca_surface_hatch` que registra despliegue controlado a superficie, teleporta a un punto seguro sobre plataforma runtime y guía hacia el escaneo ambiental.
- Resolución temporal de fuente de señal si el jugador posee `signal_resonator`.
- SaveGame con seed, versión de generador, transform, ambiente, región, mapa revelado, inventario, objetivo y bitácora.
- Build Development empaquetada en `Builds/WindowsDevelopment`.

## Última verificación (2026-09-05 — Rebuild11)

- `AstraeonEditor Win64 Development`: compilación exitosa.
- `Astraeon Win64 Development`: compilación exitosa.
- Automation Tests `Astraeon.*`: 35 encontrados, 35 exitosos, exit code 0.
- BuildCookRun Win64 Development: `BUILD SUCCESSFUL`.
- **Smoke manual real confirmado por el usuario**: ESCOTILLA funciona, recolección de recursos, crafting y resolución de señal completos en sesión jugable real (sin `-nullrhi`). Mob Umbra Grazer inflige daño al tocar al jugador, como se espera.
- Agregado en esta corrida: correr (`Shift`) y vista de bitácora (`L`).
- Pendiente reportado por el usuario tras esta corrida: piso de Ítaca superpuesto con la nueva geometría de `AAstraeonItacaInterior` (paredes/caja alrededor de Ítaca), y la ESCOTILLA no bloquea el paso físicamente (se puede salir de la caja caminando sin `E`). Ver `Docs/KNOWN_ISSUES.md`.
- **Save/close/open/continue confirmado por el usuario (2026-09-05)**: `F6` guardar → cerrar → reabrir → `F10` continuar funciona correctamente en sesión real. Cierra AC-11 para H6.

## Verificación previa (2026-09-05 — Rebuild6)

- `AstraeonEditor Win64 Development`: compilación exitosa.
- `Astraeon Win64 Development`: compilación exitosa.
- Automation Tests `Astraeon.*`: 35 encontrados, 35 exitosos, exit code 0.
- Smoke crítico editor-game: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- BuildCookRun Win64 Development: `BUILD SUCCESSFUL` después de cerrar el ejecutable viejo que bloqueaba la copia del archive.
- Smoke crítico packaged: `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.

## Entorno detectado

- SO: Microsoft Windows 11 Pro, build 26200, x64.
- Unreal Engine: `C:\Program Files\Epic Games\UE_5.7`, versión 5.7.4.
- Toolchain usado por UBT: Visual Studio 2022 MSVC 14.44.35228, Windows SDK 10.0.22621.0.
- Git: 2.50.1.windows.1.
- Git LFS: 3.7.0, filtros activos.
- Espacio libre C:: ~215 GiB, por debajo de la recomendación de 250 GiB.

## Observaciones

- Los logs de arranque del editor bajo `-NullRHI` siguen mostrando tres `LogAutomationTest: Error: Condition failed` antes de ejecutar los tests del proyecto. Los tests propios terminan en éxito; se mantiene como observación de baja severidad.
- En una corrida previa, varias pruebas fallaron porque esperaban cadenas viejas en inglés mientras el HUD actual usa español. Se corrigieron las expectativas para validar la UI real del build temporal.
- El reporte manual posterior indicó que la escotilla seguía sin funcionar. El fix Rebuild5 era insuficiente para juego real; Rebuild6 amplía el fallback a 450 cm, pondera el marcador que está cerca de la mira y aumenta la altura visual/colisión temporal de la ESCOTILLA.
- Nuevo reporte manual tras Rebuild6: el usuario está frente a `ESCOTILLA`, presiona `E` y no ocurre nada. Esto queda documentado como bloqueante en `Docs/HANDOFF_ESCOTILLA_INTERACCION.md`; el siguiente agente debe instrumentar primero para distinguir input no recibido, marker no encontrado o condición de sesión/despliegue fallida.
- Durante el primer repackage de Rebuild6, Windows bloqueó `Builds/WindowsDevelopment/Astraeon.exe` porque el ejecutable estaba abierto. Tras cerrarlo, BuildCookRun archive completó correctamente.
- El vertical slice actual es mecánico/temporal: funciona como esqueleto verificable, no como experiencia final pulida de 30–45 minutos. La brecha principal para un MVP jugable sigue siendo arte, audio, diseño de nivel, ritmo narrativo y smoke manual visual.
- El package está generado localmente en `Builds/WindowsDevelopment`, ignorado por Git.
- El commandlet de creación del mapa no debe ejecutarse sobre un asset existente sin una estrategia explícita de recreación; se observó crash al intentar regenerarlo durante una iteración previa y se evitó depender de esa vía.
- `Engine/SkyAtmosphere.h` no existe en la ruta estándar en UE5.7 con la configuración actual del proyecto; el spawn de `ASkyAtmosphere` fue removido. Si se quiere cielo físico, se debe agregar el módulo correspondiente al `Build.cs` e investigar la ruta correcta del header.

## Siguiente paso recomendado

**PRIORIDAD 1 — Bloqueante ESCOTILLA manual**

- Leer `Docs/HANDOFF_ESCOTILLA_INTERACCION.md`.
- Instrumentar `AAstraeonPlayerCharacter::Interact()` con feedback/log visible para confirmar si `E` llega, qué marker encuentra y distancia real a hatch.
- Si input no llega, evaluar binding alternativo en `AAstraeonPlayerController` evitando doble ejecución.
- Si input llega pero no encuentra marker, agregar fallback explícito por zona de ESCOTILLA.
- Recompilar, reempaquetar con `Astraeon.exe` cerrado y pedir nueva validación manual.

**PRIORIDAD 2 — Smoke manual posterior al fix de ESCOTILLA**

- Ejecutar `Builds/WindowsDevelopment/Astraeon.exe` sin `-nullrhi`.
- Nueva partida (`Enter`), acercarse a `ARGOS`, presionar `E` y confirmar que `Estado:` cambia.
- Acercarse a `ESCOTILLA`, presionar `E` una vez desde una posición razonable y confirmar despliegue a superficie sin caída.
- Escanear con click izquierdo, acercarse a cada recurso verde y confirmar recolección con `E`.
- Fabricar con `C`, ir a `SEÑAL`, interactuar con `E`, guardar con `F6`, cerrar, abrir y continuar con `F10`.
- Confirmar visualmente que bruma, rocas, mira y feedback verde son visibles.

**PRIORIDAD 3 — H5.2 restante: Ítaca/narrativa mínima**

- Reemplazar marcadores temporales por una estancia visualmente más presentable.
- Mejorar lectura/ritmo del briefing ARGOS.
- Confirmar que la transición hatch → región es clara sin depender de texto excesivo.

**PRIORIDAD 4 — H6 candidato MVP**

- Medición básica de rendimiento.
- Informe final y limitaciones conocidas.
- Ajustes derivados del smoke manual.

**No desbloqueado para este MVP:** galaxy procedural con biomas/sistemas múltiples. La visión existe en `GAME_DESIGN_MASTER.md`, pero no autoriza ampliar alcance antes de cerrar `La primera señal`.

---

## 2026-09-07 — Personaje principal: pipeline Blender cerrado

Entregado en `graphics/characters/main_player/`, vía Higgsfield Bridge sobre Blender 5.2.1 LTS.
Detalle completo y limitaciones en
[MAIN_CHARACTER_PIPELINE.md](../graphics/characters/main_player/docs/MAIN_CHARACTER_PIPELINE.md).

- Malla generada (Tripo, 10 créditos) normalizada a 1,83 m, pies en Z=0, transformaciones aplicadas.
- `SK_Astraeon_Player` 233.846 tris + LOD1/2/3 (93.538 / 37.414 / 14.030), UV y material conservados.
- `SKEL_Astraeon_Player`: 71 huesos, nomenclatura Unreal, raíz única `root`, cadenas IK y sockets.
- Skinning propio (heat weighting falla en esta malla): 0 vértices sin peso.
- 45 Actions a 30 FPS in-place; peor penetración de suela −1,2 mm, costura de loop 0 mm en los 20 cíclicos.
- Export FBX validado por round-trip: 1,830 m, 71 huesos, 45 pistas, escala 1.

**Pendiente antes de conectar al runtime:** importación real en el editor de Unreal 5.7.4 en destino
aislado. Nada de esto se ha compilado ni probado dentro del juego todavía; el casco es blockout y
los sets de textura siguen sin separar.

**Punto de reanudación, trampas verificadas y orden de trabajo:**
[HANDOFF_PERSONAJE_PRINCIPAL.md](HANDOFF_PERSONAJE_PRINCIPAL.md).
Las prioridades 1–4 del bloque anterior (bug ESCOTILLA con `E` y smoke manual) siguen abiertas:
esta sesión no tocó `Source/` ni generó build.

**Aviso de plataforma:** este Blender está localizado en español. Los nodos de material se crean con
nombre traducido (`BSDF Principista`), así que `nodes.get('Principled BSDF')` devuelve `None` y
descarta los ajustes en silencio. Buscar por `node.type` y por `socket.identifier`.

## 2026-09-08 — Protagonista: optimizado, horneado y validado en Unreal

Continuación de la sesión Codex `01a08295`, que quedó cortada a mitad del bake. Detalle
completo, evidencias y decisiones en [PENDIENTE_PROTAGONISTA.md](PENDIENTE_PROTAGONISTA.md).

**Malla.** `SK_Astraeon_Player` pasó de 233.846 a **65.284 triángulos** (100% quads,
QuadriFlow), con 0 vértices sin influencia y 4 huesos twist añadidos → 75 huesos. La cadena
de LOD se regeneró desde ese LOD0: 65.284 / 32.642 / 16.321 / 6.527. Antes estaba invertida
—LOD1 tenía 93.538, más que el LOD0 nuevo— y usaba el material atlas viejo.

**Texturas.** Sets separados Character/Suit horneados a 2048 px, más los nuevos `Gear` y
`Helmet` a 1024 px con Color/NormalGL/ORM/Emission. Tres defectos de bake resueltos: la
costura de la línea media de la cara (la cabeza es un disco topológico y se desenvuelve en
una sola isla), la fragmentación del traje (1001 islas al 34% de texel → 46%) y los parches
negros del pelo, que resultaron ser **7.954 texels con Z negativo** en el normal map, es
decir vectores apuntando hacia dentro de la superficie.

**Equipo.** Las tres piezas tenían `uv_layers = 0`. La mochila además flotaba sobre la
espalda —87 mm de hueco abajo, 51 mm de invasión arriba—; se corrigió en el generador con la
curva medida de la espalda y ahora apoya a 0,2 mm, verificado con BVH en inclinación,
torsión y extensión.

**Morphs.** Tres targets faciales autorizados: `jaw_open`, `brow_raise`, `brow_furrow`. Sin
`blink`: la cara tiene 249 vértices y ~18 por ojo, densidad insuficiente para párpados.

**Validado en Unreal 5.7.4**, destino aislado `/Game/Astraeon/Characters/Player/Optimized`
(68 paquetes): `passed: true`, 183,00 cm de altura, 4 LOD, 75 huesos, 45 clips evaluados uno
a uno, 3 morph targets y 4 materiales PBR.

**Pendiente para conectar al runtime.** Apuntar el cuerpo de sombra de
`AstraeonPlayerCharacter` al asset nuevo **rompe** `Astraeon.Art.Character.FirstPersonRigIsWired`:
las manos de primera persona viven sobre `SKEL_Humanoid_A` de 57 huesos y el protagonista
trae el suyo de 75. Se revirtió el cambio; el repositorio quedó verde. Hay que decidir entre
migrar el rig de manos al esqueleto nuevo o aceptar dos esqueletos y sustituir la prueba por
una equivalente.

**Diferido a conciencia:** retopología densa de la cabeza (habilitaría parpadeo y apertura
real de boca) y rediseño de la forma del casco.

### Integración cerrada el mismo día

`AstraeonPlayerCharacter` ya usa `/Game/Astraeon/Characters/Player/Optimized/SK_Astraeon_Player`
como cuerpo de sombra: 183,0 cm, 4 LOD, 3 morph targets, pies apoyados en el suelo de la
cápsula. Build, 55 pruebas automáticas y smoke del recorrido crítico en verde.

Se decidió convivir con **dos esqueletos** —protagonista de 75 huesos para la sombra,
`SKEL_Humanoid_A` de 57 para las manos de primera persona— en vez de unificarlos. La
medición que lo decidió: el eje que lleva el brazo al frente en este rig es **Z**, y los 45
clips lo usan entre 0,13 y 0,46, de modo que las manos quedan a medio metro del cuerpo
(`TwoHand_Idle`: x = ±0,48 m). Unificar habría exigido rehacer las poses de brazo de todo el
set. El cuerpo sólo existe para la silueta, así que dos esqueletos es suficiente y es la
arquitectura habitual en primera persona.

Quedan seis gestos nuevos autorizados en Blender (`Pulse`, `Drill`, `Hammer`, `Maul`,
`Consume`, `Present`, total 51 acciones) **sin exportar**: no hacían falta para esta ruta.
