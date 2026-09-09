# Personaje invisible en juego — investigación

Fecha de apertura: 2026-09-08. Cierre: 2026-09-08. Estado: **RESUELTO**.

Síntoma reportado por el propietario: el protagonista no se ve. Ni como sombra en primera
persona, ni con la cámara en tercera persona (tecla V). La cámara alternaba correctamente,
pero donde debería estar el personaje no había nada.

Evidencia del fallo: `Docs/evidencia/QA_TerceraPersona_SinPersonaje.png`.
Evidencia del cierre: `Docs/evidencia/QA_Personaje_TerceraPersona_Integrado.png` y
`Docs/evidencia/QA_Personaje_PrimeraPersona_Integrado.png`, ambas dentro de partida.

---

## 1. Causa raíz

**La importación FBX de animaciones sueltas perdía la escala de unidad del Armature de
Blender (metros → centímetros).**

El esqueleto importado lleva el hueso `root` a escala **100** en su pose de referencia, que
es como el FBX convierte metros a centímetros. Al importar cada clip por separado el
importador elimina el contenedor Armature y escribe las claves de `root` a escala **1**.

Consecuencia: la malla en pose de referencia medía bien —por eso todos los diagnósticos de
CPU decían «visible», con materiales asignados y bounds razonables—, pero **en cuanto se
evaluaba una animación la pose colapsaba a 1/100**: la cabeza quedaba a **1,64 cm** de los
pies en vez de 164 cm. Un personaje de centímetro y medio a 3,5 m de la cámara no ocupa ni
un píxel. El personaje siempre estuvo ahí; medía nada.

Medición que lo probó, en `Scripts/Editor/AuditCharacterPose.py`: altura de `head` menos
`root` en espacio de mundo, muestreada en cinco instantes de cada clip.

**Segunda causa, real y concurrente:** los materiales generados por script
(`M_Player_*`, `M_Human_*`) no declaraban `MATUSAGE_SKELETAL_MESH` ni
`MATUSAGE_MORPH_TARGETS`. Un material sin esa bandera no tiene shader compilado para malla
esquelética; el editor lo parchea al vuelo, el cocinado no. Corregido en el mismo lote.

---

## 2. La corrección

| Pieza | Qué hace |
|---|---|
| `Scripts/Editor/CharacterAnimationScale.py` | `normalize_root_scale()` reescribe las claves de `root` con la escala de la pose de referencia; sólo actúa si mide exactamente el desajuste 1 contra 100 y aborta ante cualquier otro caso. `validate_pose_scale()` exige altura de cabeza entre 65 y 220 cm en cinco muestras |
| `Scripts/Editor/RepairCharacterPresentation.py` | Repara de forma idempotente los **59 clips** ya importados (45 del cuerpo + 14 de manos y herramientas), fija las banderas de uso de los materiales y los recompila |
| `PrepareHumanoidBlockoutAssets.py`, `PrepareToolsBlockoutAssets.py`, `ValidateHumanoidImport.py`, `ValidateToolsImport.py`, `ValidateMainCharacterImport.py` | Normalizan y validan **en la importación**, para que el defecto no pueda volver a entrar |
| `MainCharacterAppearance.py` | Declara `MATUSAGE_SKELETAL_MESH` y `MATUSAGE_MORPH_TARGETS` antes de recompilar |

Reporte de la reparación: `ContentPipeline/reports/character_presentation_repair.json`
(`passed: true`, con las alturas de cabeza medidas clip por clip).

### Lo que se arregló en el mismo paso, ya visible el personaje

- **Equipo montado**: casco, mochila y computadora de muñeca son ahora componentes con
  `SetLeaderPoseComponent(GetMesh())`, así que siguen la pose del cuerpo sin duplicar
  animación ni multiplicar la escala de un socket.
- **Manos dentro del encuadre**: `AN_HandsFP_{Idle,Walk,Run,Jump,Land}` se generan copiando
  la pose de brazos del agarre de escáner ya autorizado sobre los clips de locomoción. Los
  brazos dejan de colgar a los costados; torso y piernas conservan su locomoción.
- **Agarre de herramienta**: el socket hereda la escala 100 de la raíz, así que el offset
  documentado en cm se divide por esa escala antes de aplicarlo y la malla queda a escala 1.

---

## 3. Descartado con evidencia por el camino

| Hipótesis | Cómo se descartó |
|---|---|
| La malla no está asignada | log: `malla=SK_Astraeon_Player`; y `player_body_wiring.json` |
| El actor o el componente están ocultos | log: `oculto=0`, `visible=1`, `registrado=1` |
| `bOwnerNoSee` deja el cuerpo invisible para su jugador | log: `ownerNoSee=0` en tercera persona |
| El brazo de cámara colapsa y mete la cámara dentro de la malla | log: `brazo_pedido=320 brazo_real=325` |
| La cámara mira a otro lado | cuerpo a 357 cm de la cámara, centrado por el brazo |
| Materiales nulos o transparentes | ambos slots asignados, `BLEND_OPAQUE`, `two_sided = false` |
| El paquete no incluye el asset | el ejecutable empaquetado reporta la misma malla y materiales |
| El escenario tapa al personaje | la captura muestra el interior despejado |
| LOD sin datos de render | descartada al medir la pose: el problema era de escala, no de nivel de detalle |
| El cuerpo se anima con clips de otro esqueleto | **era cierto y se corrigió** (ver §4); no resolvió el síntoma por sí solo |

---

## 4. Defecto anterior, real, corregido antes del cierre

`UAstraeonFirstPersonRigComponent` empujaba su clip de locomoción al cuerpo de sombra:

```cpp
ShadowBody->PlayAnimation(Sequence, bLoop);   // Sequence vive en SKEL_Humanoid_A
```

Mientras el cuerpo fue el blockout humano compartían esqueleto. Al pasar el cuerpo al
protagonista dejaron de compartirlo (57 huesos contra 75). Corregido: el cuerpo tiene su
propio juego de clips y `BodyCounterpart()` traduce; antes de reproducir se comprueba que el
esqueleto coincida. Era necesario, pero no suficiente: debajo estaba la escala.

---

## 5. Verificación del cierre

| Prueba | Resultado |
|---|---|
| `Automation RunTests Astraeon` | **55 éxitos, 0 fallos** |
| Smoke de cámara, editor | `AstraeonCharacterViewSmoke: Passed=true` en ambos sentidos, `HeadHeightCm=163.89 / 163.90` |
| Captura dentro de partida, editor | cuerpo con casco y mochila en tercera; mano con escáner en primera |
| `BuildCookRun` Win64 Development | `BUILD SUCCESSFUL` |
| Smoke de cámara, **ejecutable empaquetado** | `Passed=true` en ambos sentidos, `HeadHeightCm=163.89 / 163.90`; capturas en `Builds/WindowsProtagonista/Astraeon/Saved/Screenshots/Windows/` |
| Recorrido crítico, **ejecutable empaquetado** | `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579` |

Todo se lanza con `Scripts/RunCharacterChecks.ps1 -Check <Audit|Repair|Automation|Visual|VisualFP|Package|PackagedVisual|Critical|PackagedCritical>`.

Herramientas de diagnóstico que quedan disponibles en `AstraeonPlayerCharacter`:

| Parámetro de línea de comandos | Efecto |
|---|---|
| `-AstraeonStartThirdPerson` | arranca en tercera persona sin depender de pulsar V |
| `-AstraeonCameraShot` | inicia partida, alterna la vista con la tecla real y captura ambas |
| `-AstraeonBasicBodyMaterial` | fuerza `WorldGridMaterial` en el cuerpo (A/B de material) |
| consola `Astraeon.ToggleCamera` | alterna la vista desde `-ExecCmds` |

---

## 6. Nota de método

Se perdieron varias iteraciones deduciendo a partir de reportes de estado en vez de mirar un
fotograma, y después mirando el fotograma sin **medir la pose evaluada**. Los reportes decían
«visible» en todos los intentos porque la malla lo estaba: lo que fallaba era su tamaño una
vez animada, y ningún indicador de visibilidad lo dice.

Regla que queda: ante un asset que «está pero no se ve», medir la geometría **evaluada**
—altura de un hueso conocido, no bounds ni banderas— antes de seguir con hipótesis de render.

Trampas del arnés de diagnóstico, por si se repiten:

- `-ExecCmds` corre antes de que exista el pawn: un comando de consola que dependa del
  personaje no se ejecuta.
- Sin iniciar partida, la vista es la del menú y la captura no dice nada del personaje.
- Iniciar partida **recrea el pawn**: el estado de cámara del pawn anterior se pierde, y con
  él cualquier temporizador que se le hubiera puesto.
- Con `-AstraeonAutoSmokeCriticalPath` el juego se cierra antes de los pocos segundos que
  necesita una captura diferida.
