# Traspaso — Fase 1 cerrada, Fase 2 abierta, 2026-09-10

Estado al cerrar: **Fase 1 con su puerta pasada**, todo commiteado y fusionado en `main`.
7 commits, 123 archivos, +3.986 líneas. 75 pruebas verdes y el recorrido planetario en verde.

Lo que cambió en una frase: el planeta esférico dejó de ser un banco de pruebas y pasó a ser
terreno de dos capas caminable, con la locomoción medida y un defecto de colisión que se veía
como animación encontrado, corregido y protegido con un guardián.

Etiqueta recuperable de este cierre: **`fase-1-nucleo-planetario-cerrada`**.

---

## 1. Qué leer primero al reanudar

En este orden. Está en `AGENTS.md` §2 y no es decorativo.

1. `AGENTS.md`
2. `Docs/ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` — las 10 fases y los tres productos
3. `Docs/PLAN_TRANSICION_EJECUCION.md` — esas fases contra el código real
4. `Docs/PHASE_STATUS.md` — **qué está permitido hacer hoy**; abre la Fase 2 con su puerta
5. `Docs/ADR/0004-planetas-esfericos-fundacionales.md` y `ADR/0005-cuarentena-surfacecontract-a-fase-3.md`

`Docs/INDICE.md` dice qué es cada documento y cuánta autoridad tiene.

---

## 2. Cómo verificar que sigue en verde

```powershell
.\Scripts\RunPlanetChecks.ps1 -Check Automation   # 75 pruebas
.\Scripts\RunPlanetChecks.ps1 -Check Cardinals    # 26 direcciones: 6 caras, 12 aristas, 8 esquinas
.\Scripts\RunPlanetChecks.ps1 -Check Walk         # vuelta completa con saltos, RESULTADO=OK
.\Scripts\RunPlanetChecks.ps1 -Check Walk -Profile # lo mismo + perfil a Docs/evidencia/
.\Scripts\RunPlanetChecks.ps1 -Check Critical     # recorrido plano heredado
```

Banderas útiles del smoke de caminata, que existen porque hicieron falta para aislar un defecto:

- `-AstraeonNoJump` — camina sin saltar ni una vez. Es el **experimento de control**: si la
  locomoción se corta igual, la causa no está en el salto ni en quien lo pide.
- `-AstraeonSprint` — mantiene la carrera.

**Nunca `-nullrhi` con los scripts que spawnean clases propias** — usar `-RenderOffscreen`.

**Desde Git Bash, la ruta del mapa se mangla.** `/Game/Maps/...` se convierte en
`C:/Program Files/Git/Game/Maps/...` y el editor arranca, no encuentra el mapa y crashea en
EnhancedInput con un callstack que no dice nada. Lanzar desde PowerShell.

---

## 3. Lo que se construyó

### Núcleo planetario

| Archivo | Qué hace |
|---|---|
| `Planet/AstraeonPlanetDefinition` | Radio, masa, gravedad, nivel del mar, seeds y `GeneratorVersion`. Struct C++, no Data Asset: se difiere a la Fase 4 a propósito |
| `Planet/Coordinates/AstraeonPlanetCoordinates` | Dirección unitaria ↔ cara/UV del cube-sphere |
| `Planet/Surface/AstraeonPlanetSurface` | Relieve radial de **dos capas** + claro angular + exclusión por radio |
| `Planet/Surface/AstraeonCubeSphereMesh` | Seis caras a los tiers Lab / Target / Stress |
| `Planet/AstraeonPlanetRuntime` | Las seis caras y la colisión cercana con **doble búfer** |
| `Tests/AstraeonPlanetCardinalSmoke` | Polos, ecuador, aristas y esquinas: 26 direcciones |

### El relieve, y por qué son dos capas

La superficie radial nació como una sola capa de ±180 cm, un placeholder. Se le portaron del
dominio plano las dos capas que ese dominio ya había pagado por descubrir:

- **Suelo caminable**: amplitud contenida repartida en decenas de metros. Respeta el límite de
  escalón siempre. Medido: peor escalón **7,2 cm** contra un límite de 45.
- **Montañas**: aisladas y altas, **exentas** de ese límite a propósito. Una montaña es una
  barrera; pedirle pendiente suave sería pedirle que deje de serlo. Medido: 51 m la más alta.

Mezclarlas en una sola curva es el error que el dominio plano ya había cometido: subir la amplitud
para tener paisaje volvía el suelo intransitable, y bajarla para poder caminar dejaba el mundo liso.

Las constantes **no se re-derivaron por tier**: 700 cm de espaciado, 45 de escalón, 2.600 de claro
y 9.000 de exclusión son invariantes de escala humana. El radio del planeta cambia cuánta
superficie hay, no qué es un escalón caminable.

Un cuerpo que no da la vuelta en varias celdas de montaña **no recibe la capa**: el muestreo entero
caería dentro de una sola celda de ruido. Por eso el banco de locomoción de 200 m de radio sigue
siendo todo suelo caminable, que es lo que necesita.

`GeneratorVersion` está en **3**. La 2 era la capa única. Subirla es lo que legitima cambiar el
valor fijo de `Planet.Height.Determinism`.

---

## 4. Estado medido

| | |
|---|---|
| Pruebas | **75** verdes, 0 fallos |
| Código | 15.552 líneas C++ |
| Caminata planetaria | 250 s, 141.228 cm, 62 saltos, **0,0 %** desalineado, 0,0 % fuera de altitud |
| Cortes de locomoción | **0** frames en `Idle` mientras camina |
| Relieve | escalón de suelo 7,2 cm (límite 45); montaña más alta 5.100 cm |
| Rendimiento | 205,3 FPS medios, p99 **6,99 ms**, 1 hitch de arranque, 1920×1080 |
| Etiquetas de rescate | `pre-transicion-plana`, `fase-1-nucleo-planetario-cerrada` |

---

## 5. Trampas encontradas, para no repetirlas

Todas están en `KNOWN_ISSUES.md` con su medición. La grande de esta sesión:

**Un defecto que se ve como animación puede no serlo, y el control lo dice en una corrida.** El
propietario reportó que el caminar se veía trabado y que al correr además frenaba. La sospecha
natural era el clip. Medido con `-AstraeonNoJump`: sin saltar ni una vez, el clip oscilaba entre
`Walk_F` e `Idle` **53 veces en 40 segundos** y la velocidad tocaba exactamente 0.

La causa era `PrepareCollision`: rehacía la colisión cercana con **un solo componente**, así que
despegaba al personaje, movía el componente y recocía su cuerpo físico en el sitio. Cada 312 cm el
jugador se quedaba sin suelo dos frames. **Nunca llegaba a `Falling`**, que es exactamente por qué
todas las pruebas anteriores pasaban en verde: el defecto vivía justo por debajo de lo que medían.
Correlación 1:1 — 55 reconstrucciones de colisión, 53 cortes de animación.

Y no era sólo visual: el recorrido en 40 s pasó de 16.922 a 20.934 cm al arreglarlo. **Un 24 % más
de distancia.** El "avanza poco y se frena" era literal.

La lección operativa: **medir antes de tocar el pipeline de arte**. Una pasada de Blender cuesta
export, import y seis checks; el control costó una corrida de 40 segundos.

**Corolario para el pulido de animación**: la queja de que el caminar "no se lee natural" tenía dos
causas sumadas, y ésta era la ruidosa. Un ciclo que se reinicia tres veces por vuelta se ve
antinatural con cualquier pose.

---

## 6. Por dónde seguir

### Fase 2 — Patches, LOD, precisión y estado mutable

Entregables y puerta en `PHASE_STATUS.md` y en `PLAN_TRANSICION_EJECUCION.md` §4. Absorbe el
Bloque B de `ESTADO_Y_RUTA_MAPA.md`.

La puerta tiene ocho puntos. Dos merecen atención propia:

- **`Terrain.TraversalDetectsWalls`** existe para **exigir que el validador falle** cuando la
  región no se puede recorrer. Si al migrarla a patches hay que debilitar lo que pide para que
  apruebe, el defecto está en la migración.
- **"Una criatura abatida sigue abatida"** tras descargar y recargar su patch. Hoy `BACKLOG` MV4
  registra que las criaturas muertas reaparecen al rematerializar. Con patches que cargan y
  descargan, ese agujero pasa de molestia a bloqueo.

### Deuda que la Fase 2 hereda, con número

El parche de colisión abarca `6.0/FaceQuads` rad (~37 m a 200 m de radio) pero se rehace cada
`0.5/FaceQuads` (~3,1 m): **doce veces más seguido de lo que su tamaño exige**, recorriendo las
6.144 celdas de las seis caras y recociendo cada vez. El doble búfer quitó el corte visible, no el
desperdicio. Esta fase es la dueña del anillo de colisión.

### Lo que NO se toca

Contenido de región (Fase 3), biomas plurales y océano (Fase 4), protagonista y vuelo (Fase 5),
sistema estelar, ecología, civilizaciones y ciudades.

---

## 7. Deudas registradas, diferidas a propósito

- **`Terrain.SurfaceContract`** → Fase 3 por [ADR 0005](ADR/0005-cuarentena-surfacecontract-a-fase-3.md).
  Valida mayoritariamente contenido de región. **Se movió la fecha, no el listón**: cuando vuelva,
  vuelve entera.
- **Tres puntos del protagonista** → Fase 5, donde el documento rector ya los declaraba
  entregables: personaje sin casco, brazos de primera persona rehechos desde el protagonista
  (hoy primera persona es un blockout de 57 huesos y el cuerpo tiene 75), y herramienta visible en
  tercera persona.
- **Pulido de codos** → tarea acotada abierta. La abducción de hombro es de 9,5° sobre el brazo
  colgando y `clavicle_l/r` está en cero en toda la locomoción; el valor está escrito a mano 12
  veces en `Tools/Blender/main_character_anim.py`, así que pide una constante de módulo.
  **Volver a mirarlo antes de tocarlo**: el juicio original se hizo sobre un ciclo que se cortaba.
- **Perfiles en Data Assets** → Fase 4. **Segundo bioma** → Fase 4. **Renombrado de carpetas** →
  cuando exista el primer sistema de civilización.
- **`GroundIfRestingOnSurface`** es un workaround documentado de una limitación del motor. Si la
  Fase 2 rehace la locomoción sobre patches, conviene revisar si sigue haciendo falta.

---

## 8. Un archivo sin commitear a propósito

`graphics/characters/main_player/exports/AN_Astraeon_Player_All_Polished_20260910_before_arm_pose.fbx`
es un export intermedio, y `ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` pide no confirmar artefactos
temporales. Sirve como referencia del antes/después del pulido de brazos; si deja de hacer falta,
se borra.

## 9. El repositorio remoto

Este cierre fue **el primer push** a `github.com/maximo-cobeaga/Sidera`: hasta ahora el repositorio
existía como remoto configurado pero sin nada subido. Se subieron la historia completa y 696 MB de
objetos LFS. Conviene confirmar que la visibilidad del repositorio en GitHub es la que se pretende
antes de seguir subiendo arte.
