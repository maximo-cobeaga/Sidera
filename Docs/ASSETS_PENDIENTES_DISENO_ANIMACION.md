# Componentes pendientes de diseño y animación — ASTRAEON

**Actualización 2026-09-10 (prueba humana):** el protagonista suma tres pendientes de arte que
antes no estaban escritos, los tres de **Fase 5** y los tres ya previstos por el documento rector:
malla y animación de **personaje sin casco**, **brazos de primera persona rehechos desde el
protagonista** (hoy primera persona sigue siendo el blockout de 57 huesos) y **herramienta visible
en tercera persona**. Además, el pulido de brazos de la locomoción **no pasó la prueba humana**:
la abducción de hombro es de 9,5° y la clavícula está en cero en todos los ciclos. Detalle en
`KNOWN_ISSUES.md`, reparto en `BACKLOG.md`.

**Actualización 2026-09-10 (locomoción pulida integrada):** el cuerpo de sombra usa el lote
`Optimized_Polished` exportado desde Blender y las manos FP usan `PolishedFP`, con movimiento real
durante caminar, correr, salto y aterrizaje. La pose A de `Idle`, `Walk_*` y `Run_*` deja espacio
entre brazos y tórax; C++ conserva la fase y aplica histeresis al cambiar de locomoción. Queda
pendiente la medición de patinaje y la prueba humana; la referencia histórica de brazos congelados
ya no describe el runtime actual.

**Actualización 2026-09-06 (exterior de Ítaca y estaciones INTEGRADOS):** los §3.2 y §3.3 de
este documento ya no describen el estado actual. Casco, motores, patines, antena, hoja de
escotilla, consola de pilotaje y mesa de fabricación existen y están montados. Ver
`ITACA_EXTERIOR_INTEGRATION.md`.

**Actualización 2026-09-06 (criatura INTEGRADA):** el §3.6 y el §4.3 tampoco. `Umbra Grazer`
tiene modelo, esqueleto de 37 huesos, variante y siete animaciones de estado, incluidas daño
y muerte. Ver `CREATURE_ART_INTEGRATION.md`.

**Actualización 2026-09-08 (protagonista TERMINADO E INTEGRADO):** el personaje jugable ya no
es trabajo pendiente. Malla de 65.284 triángulos con 4 LOD coherentes, sets de textura
separados Character/Suit/Gear/Helmet, equipo con UV y mochila ajustada a la espalda, tres
morph targets faciales, 45 clips validados en Unreal y conectado a `AstraeonPlayerCharacter`.
Ver `PENDIENTE_PROTAGONISTA.md`. Sigue pendiente sólo el rediseño de la forma del casco.

**Actualización 2026-09-06 (personaje y herramientas INTEGRADOS):** el §3.1 y el §3.5 tampoco.
Manos FP, herramienta en mano, sombra propia y catorce animaciones están montadas sobre
`AAstraeonPlayerCharacter`. Ver `CHARACTER_TOOLS_INTEGRATION.md`.

Lo pendiente en todo lo anterior es calidad, no existencia: IK de pies, mezcla por velocidad,
materiales de producción, equipo intercambiable, cabina real, y la revisión humana.

Sigue pendiente y sin empezar, por orden de lo que más se nota jugando:

1. **Terreno, rocas y cielo**: bloques de 12 m instanciados, cubos ocres, sin skybox ni
   atmósfera física.
2. **Animación de despegue y aterrizaje**: hoy la estancia se oculta y aparece la nave, sin
   transición. Los patines no se repliegan; los motores no tienen ciclo.
3. **VFX**: empuje de motores, polvo de aterrizaje, pulso e impacto del arma, haz del escáner,
   chispas del taladro, daño al traje. Motores y patines ya tienen anclaje.
4. **Audio**: cero reproducciones de sonido en todo el proyecto.
5. **UMG**: toda la interfaz sigue siendo texto dibujado en `AAstraeonHUD::DrawHUD`.



**Actualización posterior del 2026-09-06 — humano:** `SKEL_Humanoid_A`, cuerpo vestido,
manos FP y siete animaciones simples ya están producidos en Blender Q1 y verificados
mediante importación Unreal transitoria. Ver `HUMANOID_ART.md`. Las menciones siguientes
a cero esqueletos/animaciones describen el runtime y la auditoría previa: aún falta
conectar este lote al Character, guardar sus paquetes y verificarlo jugando.

Fecha: 2026-09-06. Inventario levantado **desde el código**, no desde el plan previo, para
reflejar el estado real después de la fase de mundo vivo (nave pilotable, mesa de
fabricación, combate, herramientas, relieve).

**Actualización 2026-09-06:** siete mallas originales Q1 reemplazan los cubos visibles de
fibra, ferrita, recurso de seed, dos vetas, señal y anomalía. Fuente reproducible en
`Tools/Blender/generators/region_blockout.py`, assets en `Content/Astraeon/Art/Blockouts/Region/`.
Ver `REGION_ART_INTEGRATION.md` para pruebas y limitaciones. Manos, esqueletos, animaciones,
VFX, audio y UMG siguen pendientes.

## 0. Estado de partida, sin adornos

Verificado por búsqueda en `Source/`:

- **0 skeletal meshes, 0 esqueletos, 0 animaciones.** No existe `SkeletalMesh`,
  `AnimInstance` ni `AnimSequence` en ninguna parte.
- **0 VFX.** No hay Niagara ni sistemas de partículas.
- **0 audio.** No hay `USoundBase`, `AudioComponent` ni una sola reproducción de sonido.
- **0 UMG.** Toda la interfaz es texto dibujado en C++ (`AAstraeonHUD::DrawHUD`).
- Las excepciones a las primitivas del motor son ahora el kit de Ítaca y los siete
  blockouts regionales; personaje, criatura, nave y estaciones restantes siguen pendientes.

Esto no es una crítica del trabajo hecho: el proyecto priorizó deliberadamente validar el
bucle mecánico antes que el arte, y ese bucle **está validado y jugable**. Pero significa
que la capa de presentación arranca casi de cero.

## 1. Lo que YA existe (no rehacer)

Kit de blockout Q1 en `Content/Astraeon/Art/Blockouts/Itaca/`, generado por el pipeline de
Blender documentado en `BLENDER_PIPELINE.md`:

| Asset | Uso actual en código |
|---|---|
| `SM_Itaca_Floor_200_Blockout` | Suelo y techo de `AAstraeonItacaInterior` |
| `SM_Itaca_Wall_200_Blockout` | Paredes laterales, fondo y frente |
| `SM_Itaca_HatchFrame_Blockout` | Marco de la escotilla |
| `SM_Itaca_ARGOSConsole_Blockout` | Consola ARGOS (`AAstraeonRegionMarker`) |
| `SM_Ref_Human_180_Blockout` | Referencia de escala, no personaje |
| `M_Blockout_*` (4 materiales) | Materiales de blockout |

Estos están en **Q1 (blockout legible)**. Falta subirlos a Q2/Q3 según
`ART_ASSET_MASTER_PLAN.md`, pero no hay que modelarlos de nuevo.

## 2. Especificaciones de escala vigentes

Sacadas del código; **respetarlas o el gameplay se rompe**.

| Elemento | Medida real en juego |
|---|---|
| Cápsula del personaje | radio 42 cm, semialtura 96 cm → **1,92 × 0,84 m** |
| Altura de cámara (ojos) | Z +64 cm relativo → **~1,60 m** del suelo |
| Escalón máximo caminable | **45 cm** (`AAstraeonTerrainField::GetMaxWalkableStepCm`) |
| Estancia de Ítaca | módulo **8 × 6 × 2,8 m**, grilla 2 m, panel 0,12 m |
| Hueco de escotilla | **1,3 × 2,2 m** en marco de 1,8 × 0,24 × 2,5 m |
| Casco de la nave (placeholder) | **9 × 6 × 2,2 m** |
| Criatura Umbra Grazer | **1,4 × 0,8 × 0,7 m** (esfera escalada) |
| Bloques de terreno | **12 m** de lado, hasta **10 m** de alto |
| Rocas procedurales | 0,6–3,0 m de planta, 0,25–1,55 m de alto |

---

## 3. MODELADO pendiente

### 3.1 Personaje jugable — resuelto el 2026-09-08

> **Actualización 2026-09-08.** Esta sección quedó obsoleta: el protagonista existe
> (65.284 tris, 4 LOD, 75 huesos, esqueleto propio `SK_Astraeon_Player_Skeleton`), está
> montado en `AAstraeonPlayerCharacter` con casco, mochila y computadora de muñeca, y se ve
> en juego en primera y tercera persona. Las manos de primera persona siguen sobre
> `SKEL_Humanoid_A`, que sí existe. Lo que queda es calidad de animación (§4.1) y las dos
> tareas de forma de `PENDIENTE_PROTAGONISTA.md` §3. Se conserva el texto original abajo
> como registro del punto de partida.

Hoy **no hay ningún mesh**: el jugador es una cápsula invisible con una cámara.

- Manos en primera persona (lo mínimo imprescindible: sostienen escáner, cortadora y
  taladro).
- Traje: torso, piernas, botas, guantes, casco, mochila.
- Cuerpo completo sólo si se quiere sombra propia o vista en tercera persona.
- **Esqueleto `SKEL_Humanoid_A`** — no existe todavía; es prerrequisito de toda animación
  humana.

**Dónde enchufa**: `AAstraeonPlayerCharacter`, que hoy sólo tiene cápsula + cámara.

### 3.2 Ítaca exterior / nave — nuevo desde la fase de mundo vivo

La nave ahora **vuela y aterriza**, y su casco es un cubo gris escalado.

- Casco exterior coherente con la estancia interior de 8 × 6 × 2,8 m.
- Motores / toberas (con puntos de anclaje para VFX de empuje).
- Tren de aterrizaje.
- Escotilla exterior como **hoja articulada** (ver §4: el propietario ya pidió que no sea
  una puerta siempre abierta).
- Cabina/puesto de pilotaje visible desde el interior.

**Dónde enchufa**: `AAstraeonShipPawn::HullMesh` (`Source/Astraeon/Private/Ship/`), marcado
en el código como placeholder a reemplazar.

### 3.3 Estaciones interiores de Ítaca

| Estación | Estado | Id en código |
|---|---|---|
| Consola ARGOS | blockout Q1 hecho | `itaca_argos_console` |
| **Consola de pilotaje** | **cubo** | `itaca_pilot_console` |
| **Mesa de fabricación** | **cubo** | `itaca_fabricator` |
| Escotilla | marco Q1, sin hoja | `itaca_surface_hatch` |

### 3.4 Recursos y puntos de interés — blockouts originales Q1 integrados

| Elemento | Aspecto actual | Id |
|---|---|---|
| Fibra | haz de siete filamentos, 80 × 80 × 100 cm | `silicate_fiber` |
| Ferrita | nódulos facetados, 80 × 80 × 100 cm | `ferrite_nodule` |
| Recurso característico de seed | grupo cristalino compartido provisional, 110 × 110 × 115 cm | cuatro ids existentes, sin renombrar |
| **Vetas profundas** | collar mineral con núcleo retraído, 110 × 110 × 115 cm | `cryo_ferrite_vein`, `resonant_quartz_vein` |
| Fuente de señal | dos anillos cruzados y núcleo, 160 × 160 × 220 cm | `signal_source` |
| Anomalía menor | cuatro estratos girados, 100 × 100 × 140 cm | `minor_geologic_anomaly` |

Nota de diseño: las vetas profundas deben **leerse como inalcanzables a mano** desde lejos,
porque exigen taladro. El collar envolvente ya las diferencia de los recursos sueltos;
falta validar esa lectura con una persona. Los dos subtipos comparten silueta exterior y
se distinguen sobre todo por paleta/núcleo. `E` ahora conserva el feedback `tool_core_drill`,
antes ocultado por el mensaje genérico de interacción fallida.

Las medidas nuevas son de presentación; los proxies C++ de colisión/interacción mantienen
sus dimensiones anteriores. Faltan colisión ajustada a silueta, materiales de producción y
formas específicas para cada recurso de seed (el proxy cristalino no define su composición).

### 3.5 Herramientas y equipo — sin ningún visual

Existen como ítems de inventario y no se ven nunca:

- **Cortadora de pulso** (`weapon_pulse_cutter`): modelo en primera persona.
- **Taladro de núcleo** (`tool_core_drill`): modelo en primera persona.
- **Escáner**: se usa desde el primer minuto y nunca tuvo modelo.
- **Resonador de señal** (`signal_resonator`): objeto de misión, nunca se ve.
- **Módulos de protección** (respirador, aislante térmico, sellado de presión): idealmente
  variantes visibles del traje, no sólo una línea de inventario.

### 3.6 Criatura — `Umbra Grazer`

Hoy es una **esfera escalada** que cambia de tamaño según su estado de alerta.

- Modelo del cuadrúpedo (silueta baja y ancha, 1,4 m de largo).
- **Esqueleto y skinning** — no existen.
- Al menos una variante visual (lo exige AC-06).
- Estados de daño / cadáver, ahora que el combate es letal.

### 3.7 Entorno y terreno

- Material y meshes de suelo reales (hoy: placa gris de 1200 × 1200 m).
- Terreno de relieve: hoy son **bloques de 12 m instanciados**. Necesita meshes que lean
  como roca, respetando el escalón de 45 cm.
- Rocas y landmarks de región (hoy: cubos ocres).
- Cielo: no hay skybox ni atmósfera física, sólo luz direccional + bruma.

---

## 4. ANIMACIÓN pendiente

Ordenado por lo que más se nota en juego. El personaje jugable ya tiene su set integrado
y es la excepción: ahí el pendiente es de calidad, no de existencia. El resto no existe.

### 4.1 Personaje — existe y está integrado; falta calidad (actualizado 2026-09-08)

Ya **no** está bloqueado ni vacío: el protagonista tiene 45 clips propios integrados al
`AstraeonPlayerCharacter`, visibles en juego en primera y tercera persona. Lo que falta es
pulido, reportado por el propietario al probar el ejecutable: **camina raro, salta raro y
los brazos se ven mal**.

- **Poses de brazo del set de locomoción.** Los 45 clips posan los brazos en cruz, no al
  costado ni al frente (medido: el eje que lleva el brazo al frente es Z y los clips lo usan
  entre 0,13 y 0,46; manos a x = ±0,48 m). Se autorizaron para validar el pipeline.
- **Brazos de primera persona congelados.** `AN_HandsFP_*` copia la pose de agarre de
  escáner sobre todo el clip: metió las manos en el encuadre, pero no se mueven al caminar.
- **Salto.** Hoy es un clip suelto; falta separarlo en despegue, vuelo y aterrizaje.
- **Contacto de pies.** Sin medir el patinaje contra la velocidad real de `CharacterMovement`.

Siguen sin existir, ordenados por lo que más se nota:

- **Interactuar** (`E`) — el gesto más repetido del juego.
- **Escanear** (click izq.) — debe distinguirse claramente de disparar.
- **Disparar** la cortadora (click der.) + retroceso.
- **Extraer** con el taladro sobre una veta profunda: es la única acción con duración
  implícita del juego y hoy es instantánea y muda.
- Equipar/cambiar módulo de protección (`1`/`2`/`3`).

Seis gestos nuevos (`Pulse`, `Drill`, `Hammer`, `Maul`, `Consume`, `Present`) están
autorizados en Blender y **sin exportar**. Ficha del pendiente de calidad en
`KNOWN_ISSUES.md`; la normalización de escala de raíz ya está enganchada a la importación,
así que reexportar es seguro.

### 4.2 Nave Ítaca

- **Despegue** y **aterrizaje** (hoy: la estancia se oculta y aparece la nave, sin
  transición).
- Tren de aterrizaje desplegando/replegando.
- **Hoja de escotilla abriendo/cerrando** — el propietario ya observó que hoy funciona
  "como una puerta siempre abierta"; ver `KNOWN_ISSUES.md`.
- Ciclo de motores en vuelo / ralentí.

### 4.3 Criatura

- Pastar (idle), patrullar (marcha).
- **Alerta** y **amenaza** — hoy sólo cambia de escala, que es un placeholder evidente.
- Recibir daño y **muerte** (nuevo: el combate letal ya está implementado).
- Retirada / desinterés.

### 4.4 Estaciones y objetos

- Pantallas de consolas ARGOS / pilotaje / fabricación (animación de bucle).
- Feedback de fabricación en la mesa (hoy: sólo una línea de texto).
- Recolección de una veta.

---

## 5. VFX y audio — capas enteras inexistentes

**VFX**: pulso y impacto del arma, haz del escáner, chispas del taladro, empuje de motores,
polvo de aterrizaje, atmósfera/partículas ambientales, feedback de daño al traje.

**Audio**: no hay **una sola** reproducción de sonido en el proyecto. Pasos, viento, motores,
disparo, impacto, criatura, interfaz, alarma de oxígeno bajo.

## 6. Interfaz — reemplazo completo de UMG

Todo el HUD es texto dibujado en C++ (`AAstraeonHUD`). Vistas ya implementadas
funcionalmente que necesitan diseño real:

- HUD de estado (ambiente, amenazas, protección equipada).
- **Inventario** (`I`), **bitácora** (`L`), **mesa de fabricación** (`E` en la estación),
  **HUD de vuelo** (altitud, velocidad, techo atmosférico).
- Menú inicial y selector de seed.
- Mira central y feedback de interacción.

---

## 7. Orden sugerido para la sesión de arte

1. **`SKEL_Humanoid_A` + manos en primera persona: fuente Q1 hecha.** Siete animaciones
   simples e importación transitoria probadas; integración jugable pendiente.
2. **Herramientas en primera persona** (escáner, cortadora, taladro) con sus animaciones de
   uso: son las tres acciones centrales del bucle.
3. **Criatura**: modelo, rig y las cinco animaciones de estado. Cierra AC-06, que hoy se
   sostiene con una esfera que cambia de tamaño.
4. **Exterior de Ítaca + hoja de escotilla + despegue/aterrizaje.** Es la incorporación más
   reciente y la más vistosa.
5. **Estaciones interiores** (pilotaje y fabricación) para igualar a la consola ARGOS.
6. Terreno, rocas y cielo.
7. UMG, VFX y audio.

## 8. Restricciones que el arte no debe romper

- **No cambiar los `FName` de ítems, recursos ni marcadores**: son claves de guardado y de
  recetas (`silicate_fiber`, `itaca_fabricator`, `weapon_pulse_cutter`…). Cambiar la
  presentación sí, el id no.
- Respetar el **escalón de 45 cm** del terreno; hay un test que lo verifica
  (`Astraeon.WorldGen.Terrain.Relief`).
- La escotilla debe conservar un **hueco real de 1,3 × 2,2 m** para que pase la cápsula.
- La estancia de Ítaca **se mueve** (la nave aterriza donde el jugador quiera): nada puede
  asumir que Ítaca está en el origen del mundo.
- Los marcadores usan `TextRenderComponent` para su rótulo; al reemplazarlos por arte hay
  que decidir si el rótulo sobrevive como diegético o pasa a UMG.
