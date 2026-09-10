# Traspaso — transición a planetas esféricos, 2026-09-09

Estado al cerrar: **Fase 0 con su puerta pasada**, todo commiteado, árbol limpio.
12 commits, 58 archivos, +6.556 líneas. 69 pruebas verdes y recorrido plano crítico en verde.

Lo que cambió en una frase: el proyecto dejó de construir un mundo plano y pasó a construir el
núcleo planetario esférico primero, con la documentación sincronizada y un banco de pruebas de
gravedad radial que funciona y está medido.

---

## 1. Qué leer primero al reanudar

En este orden. Está en `AGENTS.md` §2 y no es decorativo: los rectores se corrigieron esta sesión
y leer el equivocado da dirección anulada.

1. `AGENTS.md`
2. `Docs/ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` — las 10 fases y los tres productos
3. `Docs/PLAN_TRANSICION_EJECUCION.md` — esas fases contra el código real
4. `Docs/PHASE_STATUS.md` — **qué está permitido hacer hoy**
5. `Docs/ADR/0004-planetas-esfericos-fundacionales.md`

`Docs/INDICE.md` dice qué es cada uno de los 47 documentos y cuánta autoridad tiene.

---

## 2. Cómo verificar que sigue en verde

```powershell
.\Scripts\RunCharacterChecks.ps1 -Check Automation   # 69 pruebas
.\Scripts\RunCharacterChecks.ps1 -Check Critical     # recorrido plano completo

# Caminata planetaria: vuelta a la esfera con saltos, 0 desalineación
UnrealEditor-Cmd.exe Astraeon.uproject /Game/Maps/TL_10_RadialGravity -game `
    -unattended -nosplash -RenderOffscreen -AstraeonSmokePlanetWalk -AstraeonWalkSeconds=150

# Baseline de rendimiento (escribe JSON en Docs/evidencia/)
UnrealEditor-Cmd.exe Astraeon.uproject /Game/Maps/L_AstraeonBootstrap -game `
    -unattended -nosplash -RenderOffscreen -AstraeonPerfBaseline

# Calibración Blender -> Unreal, las dos mitades
blender.exe --background --python .\Tools\Blender\calibration_ue57.py
UnrealEditor-Cmd.exe Astraeon.uproject -unattended -nosplash -RenderOffscreen `
    -ExecutePythonScript=.\Scripts\Editor\CreateAssetCalibrationMap.py
```

**Nunca `-nullrhi` con los scripts que spawnean clases propias** — ver §5.

---

## 3. Lo que se construyó

### Documentación: seis rectores sincronizados

El bloqueo real era `AGENTS.md` §5, que prohibía literalmente *"planetas esféricos totalmente
transitables"*: cualquier sesión que empezara leyendo los rectores encontraba la instrucción
contraria a lo que iba a construir.

Ninguno se reescribió. Cada uno declara en su cabecera qué conserva y qué quedó superado, porque
borrarlos perdería el porqué del código actual.

| Documento | Qué pasó |
|---|---|
| `AGENTS.md` | Misión por fases, orden de lectura nuevo, **§4.1 restricciones planetarias** normativas, §5 sin la prohibición, §7 cuarentena acotada, §8 presupuesto por percentiles, §9 seeds en workers, §11 identidad planetaria, §15 cierre por fase |
| `MVP_LA_PRIMERA_SENAL.md` | Sigue siendo el alcance jugable, ahora destino de la Fase 3. Único punto derogado: §3.3 "no requiere planeta esférico completo", tachado en su sitio |
| `GAME_DESIGN_MASTER.md` | La visión no se recorta; se adelanta su núcleo técnico. El contenido sigue tardío |
| `ASTRAEON_PLAN_DESARROLLO_MAPA.md` | Orden invertido en su primer tramo. Se conservan las cuatro propiedades del §1, seeds y contratos |
| `GUIA_ARTE_PLANETAS_BLENDER.md` | Su supuesto técnico **anticipó** la dirección actual y pasó de supuesto a requisito. Sólo caducaron los radios, reclasificados como laboratorio |
| `ESTADO_Y_RUTA_MAPA.md` | Bloque A vigente, Bloque B absorbido por la Fase 2, Bloque C cancelado |

Nuevos: `ADR/0004`, `PLAN_TRANSICION_EJECUCION.md`, `PHASE_STATUS.md`, `INDICE.md`.

### Código: módulo `Planet/`

| Archivo | Qué hace |
|---|---|
| `Planet/Coordinates/AstraeonPlanetFrame` | Arriba radial, dirección de gravedad, altitud, proyección tangente, alineación progresiva, **transporte del frente** y yaw alrededor del arriba local. Funciones puras, sin mundo ni tick |
| `Planet/Gravity/AstraeonPlanetGravityComponent` | `SetGravityDirection` + orientación de cápsula + marco de mirada local + re-apoyo tras salto. **Dormido si el mapa no tiene harness**, así que viaja montado en el protagonista sin tocar el recorrido plano |
| `Planet/Gravity/AstraeonPlanetGravityHarness` | La esfera de pruebas del §1.2. En C++ y no como Blueprint: no existía ninguna esfera previa que renombrar —el documento asumía una que este repositorio nunca tuvo— y `AGENTS.md` §4 reserva los BP para presentación |
| `Debug/AstraeonPerfBaseline` | Subsistema de mundo: mide por percentiles y hitches y escribe JSON con su línea de comandos |
| `Tests/AstraeonPlanetWalkSmoke` | Camina la esfera con saltos y mide vuelco, caída, racha en el aire, clip elegido y altitud |

### Herramientas

- `Tools/Blender/presets/UE57_AST_V1.py` — el preset de exportación deja de estar duplicado en
  dos archivos. La calibración comprueba en cada corrida que no ha derivado.
- `Tools/Blender/calibration_ue57.py` y `Scripts/Editor/CreateAssetCalibrationMap.py`
- `Scripts/Editor/CreateRadialGravityTestMap.py`

### Mapas y evidencia

`TL_10_RadialGravity` (radio 200 m, cuatro balizas sin colisión), `TL_00_AssetCalibration`,
`Docs/evidencia/BASELINE_RENDIMIENTO.md` y `CALIBRACION_BLENDER_UNREAL.md` con sus JSON.

---

## 4. Estado medido

| | |
|---|---|
| Pruebas | **69** verdes, 0 fallos (58 previas intactas + 9 del marco planetario + 2 del baseline) |
| Código | 14.284 líneas C++ |
| Caminata planetaria | Vuelta completa a la esfera, antípoda cruzado, **0,0 %** de frames desalineados, racha en el aire de **1,02 s** |
| Calibración | Cubo 100,000 cm · ejes 50/35/20 sin permutar · mannequin 183,000 cm · morph presente · animación 1,000 s |
| Rendimiento | Plano 289,8 FPS medios y p99 de 5,19 ms; harness 299,0 y 4,50 ms. Cero hitches |
| Etiqueta de rescate | `pre-transicion-plana` sobre `503e3a1` |

---

## 5. Trampas encontradas, para no repetirlas

Todas están en `KNOWN_ISSUES.md` con su medición. Las que más tiempo costaron:

**`-nullrhi` mata al editor al spawnear cualquier actor propio con malla.** Perseguí mi propio
constructor cuatro iteraciones antes de correr el control que lo resolvió en una:
`AAstraeonRegionMarker`, que lleva meses en el juego, crashea idéntico. **El control debió ser el
primer experimento, no el quinto.** Los scripts que spawnean clases propias usan
`-RenderOffscreen`.

**`new_level` crea el nivel pero el mundo activo puede seguir siendo `L_AstraeonBootstrap`.** Un
`save_current_level()` detrás lo sobrescribiría. Los generadores ahora verifican el mundo activo
y abortan si no coincide — y el guardián **saltó de verdad** una vez.

**`set_collision_enabled` no persiste.** Altera el estado en memoria, no la `BodyInstance` que se
serializa: hay que cambiar el **perfil**. El generador relee tras guardar y falla si alguna baliza
sigue bloqueando. Es la misma lección que los materiales del protagonista.

**Una calibración no puede reimportar sobre lo que dejó la corrida anterior.** El primer intento
lo hacía y perdió los morph targets: el informe habría culpado al exportador de un defecto del
estado previo.

**Interchange ignora las opciones de `FbxImportUI`** y con un FBX de sólo armature dice "no había
datos que importar". Los importadores del protagonista y de la criatura ya lo desactivaban.

**Al desplazarse tangencialmente sobre una superficie convexa, el contacto es rasante y el motor
lo descarta como roce de pared, no como aterrizaje.** El personaje quedaba en caída indefinida
—racha de 116 s— deslizándose con el clip de salto puesto. Lo aisló un experimento simple:
saltando quieto aterriza siempre, saltando en movimiento no aterriza nunca.

**`pose_bone.location` está en espacio local del hueso**, donde Y corre a lo largo del hueso.
Animar Z en un hueso vertical lo mueve de costado.

---

## 6. Por dónde seguir

### Antes de nada, un minuto de partida

El arreglo del salto en movimiento está medido pero **no probado a mano**, y es lo último que se
tocó de algo que ya te funcionaba. Abrí `TL_10_RadialGravity`, corré, saltá y desplazate en el
aire. Si el clip de salto ya no se traba, la locomoción del harness queda cerrada.

### Fase 1 — Núcleo planetario

Su puerta y entregables están en `PLAN_TRANSICION_EJECUCION.md` §4. En orden:

1. `FAstraeonPlanetDefinition` — radio, masa, gravedad, nivel del mar, seeds, versión.
   **Struct C++, no Data Asset**: se difiere a la Fase 4 a propósito (§6 del plan).
2. `FAstraeonPlanetCoordinates` — `FaceUvToCube`, `CubeToFaceUv`, dirección ↔ cara/UV.
3. **Migrar `AstraeonTerrainField` al dominio radial.** Es el trabajo grande y el más preparado:
   `GetHeightCm` ya es una función estática y pura de `(seed, x, y)`, escrita a propósito para que
   la generación planetaria la consultara antes de que el terreno existiera. Cambia la firma y el
   dominio del ruido, no la lógica.
4. `APlanetRuntime` con las seis caras a LOD bajo y `TL_11_CubeSphereClosed`.
5. Tiers Lab 10 km / Target 500 km / Stress 2500 km. **El radio es un dato, no un `Scale`.**

Pruebas que pide la fase: `Planet.Topology.FaceEdgesMatch`, `Planet.Height.Determinism`,
`Planet.Gravity.CardinalPoints`. Al cerrarla salen de cuarentena `Terrain.Relief` y
`Terrain.SurfaceContract`.

### Lo que NO se toca

Biomas plurales, océano y atmósfera, nave pilotable, sistema estelar, ecología, civilizaciones y
ciudades. Y el arte sigue en Q1: el pulido de animación de `KNOWN_ISSUES` entra como tarea acotada
dentro de una fase, no como fase propia.

---

## 7. Deudas registradas, diferidas a propósito

En `PLAN_TRANSICION_EJECUCION.md` §6, con su motivo:

- **Perfiles en Data Assets** → Fase 4. Sus estructuras cambian de forma durante las Fases 1 y 2;
  convertirlas hoy sería convertirlas tres veces.
- **Segundo bioma** → Fase 4. Es el punto once del primer gran hito, pero pedirlo sobre geometría
  que va a cambiar es pedirlo dos veces.
- **Renombrado de carpetas** al esquema del documento rector → cuando exista el primer sistema de
  civilización. El módulo `Planet/` ya nace con la estructura correcta.
- **`GroundIfRestingOnSurface`** es un workaround documentado de una limitación del motor, no una
  solución de diseño. Si la Fase 2 rehace la locomoción sobre patches, conviene revisar si sigue
  haciendo falta.
