# Protagonista principal — estado real y pendientes

Última auditoría: 2026-09-08
Fuente canónica: `graphics/characters/main_player/blender/CHR_Astraeon_Player.blend`
Método: inspección directa de la escena viva en Blender (Higgsfield Bridge) + reportes en disco.
Origen: continuación de la sesión Codex `01a08295-325d-7a42-a145-f583b434625d`, interrumpida antes del cierre.

---

## 1. Hecho y verificado

### Heredado de la sesión Codex

| # | Entregable | Evidencia |
|---|---|---|
| H1 | Import a Unreal del personaje original: `passed: true`, `saved_packages: true`, 4 LOD, 71 huesos, 45 clips | `ContentPipeline/reports/main_character_unreal_import.json` |
| H2 | Fix de las 7 poses de agarre que Unreal descartaba por durar 1 fotograma | `Tools/Blender/main_character_export.py` `prepare_grip_export_samples` |
| H3 | Retopología QuadriFlow: **65.284 tris / 32.642 quads (100% quads)**, dentro del objetivo 60–80k | `docs/retopo_progress.json` |
| H4 | Transferencia de pesos: **0 vértices sin influencia**; 4 huesos twist → 75 huesos | `docs/skin_transfer.json`, `docs/skin_polish.json` |
| H5 | Separación de materiales `MAT_Player_Character` / `MAT_Player_Suit` | escena viva |

### Cerrado en esta sesión

| # | Entregable | Evidencia |
|---|---|---|
| **P8** | `.blend` canónico **guardado**; 2 checkpoints nuevos en `Saved/BlenderRecovery/` | `CHK_Player_before_uv_rebuild_20260908.blend`, `CHK_Player_rebake_lod_equip_20260908.blend` |
| **P2** | Costura facial y artefactos de bake **resueltos** | `QA_Rebake_Face_v2.png`, `docs/uv_rebuild.json`, `docs/normal_clamp.json` |
| **P1** | LOD1–3 **regenerados** desde el LOD0 nuevo | `docs/lod_rebuild.json` |
| **P3** | UV del equipo creadas (las tres piezas tenían `uv_layers = 0`) | `docs/uv_equipment.json` |
| **P11** | Mochila **asentada sobre la espalda real** (defecto nuevo hallado en QA) | `docs/equipment_fit.json`, `QA_Backpack_Fit_Side.png` |
| **P5** | Export FBX completo: malla nueva, 3 LOD, casco, mochila, computadora, 45 animaciones y 8 texturas | `docs/export_progress.json` |
| **P7** | Auditoría de deformación superada | `docs/deformation_audit.json`, `QA_Deform_*.png` |
| **P4** | 3 morphs faciales autorizados y verificados | `docs/facial_morphs.json`, `QA_Morph_*.png` |
| **P12** | Sets `Gear` y `Helmet` horneados + sombreado suave a 35° | `docs/equipment_bake.json`, `QA_Helmet_Smooth.png` |
| **P6** | **Validación en Unreal SUPERADA**: `passed: true`, `saved_packages: true` | `ContentPipeline/reports/main_character_unreal_optimized.json` |
| **P9** | **Integrado al juego**: el personaje es el cuerpo de sombra; build, 55 pruebas y smoke en verde | `ContentPipeline/reports/player_body_wiring.json` |

#### P2 — qué era y qué se hizo

Tres causas distintas, diagnosticadas por separado:

1. **Costura vertical en la línea media de la cara.** `smart_project` partía la cabeza en
   68 islas, una de ellas cortando de frente a mentón. La región `MAT_Player_Character`
   resultó ser una sola componente conexa con característica de Euler = 1, es decir un
   **disco topológico**: puede desenvolverse en **una isla sin ninguna costura interior**.
   Resultado: 68 → **1 isla**, cobertura UV 69%, 0 caras invertidas.
2. **Fragmentación del traje.** 1001 islas usando ~34% del texel → `average_islands_scale`
   + `pack_islands` denso: **cobertura 46%**, estiramiento uniforme (0,13–0,21).
3. **Parches negros en el pelo.** No eran texels sin hornear (verificado: **0 huecos
   interiores** en Color y ORM), ni normales invertidas, ni agujeros de malla (20 aristas de
   borde, todas en piernas y botas). Eran **normales tangenciales degeneradas**: el bake
   proyecta una fuente de 233k con grietas profundas sobre una superficie suave y guardaba
   **7.954 texels con Z < 0** — vectores apuntando hacia dentro de la superficie, que EEVEE
   y Unreal sombrean en negro. Se acotó Z a 0,40 y se renormalizó XY: 1,13% de los texels
   de Character y 2,66% de Suit corregidos.

Además: re-bake completo con cage 8 mm, ray 30 mm, margen 16 px `ADJACENT_FACES` e
inpaint iterativo (Character 164.536 texels rellenados, Suit 578.147).

**Limitación conocida, no bug:** quedan unas pocas muescas oscuras en la silueta del pelo.
Son pérdida de fidelidad de la retopo — el arte original de 233k resuelve mechones que
65k no puede. Comparativa: `QA_Compare_Source_Face.png`.

#### P1 — cadena de LOD

Antes: `LOD1 = 93.538 tris`, **mayor que el LOD0 nuevo de 65.284**, y los tres con el
material atlas viejo. Ahora, por colapso desde el LOD0 definitivo:

| Nivel | Triángulos | Materiales | UV | Grupos de vértices | Sin influencia |
|---|---|---|---|---|---|
| LOD0 | 65.284 | Character + Suit | 1 | 56 | 0 |
| LOD1 | 32.642 | Character + Suit | 1 | 56 | 0 |
| LOD2 | 16.321 | Character + Suit | 1 | 56 | 0 |
| LOD3 | 6.527 | Character + Suit | 1 | 56 | 0 |

Monótona, mismo conjunto de grupos que LOD0, altura 1,83 m preservada en los cuatro.
Los LOD viejos se archivaron como `ARCHIVE_*_Legacy`, no se borraron.

#### P11 — ajuste de la mochila (defecto nuevo)

`QA_Rebake_Body_Side.png` mostró la mochila **flotando separada de la espalda**. Medido:
la pieza tenía un frente plano (y ≈ 0,14–0,16) contra una espalda que va de y = 0,074 en
la cintura a y = 0,191 en los omóplatos → **87 mm de hueco abajo y 51 mm de invasión
arriba**.

Se corrigió en el generador, no en la malla: se incorporó la curva medida de la espalda
(`BACK_CURVE`) y un arco propio para la cara trasera de la carcasa, de modo que el frente
sigue el cuerpo con 4 mm de holgura y el espesor varía por anillo. Verificado con BVH
contra el cuerpo evaluado: **contacto a 0,2 mm en reposo**, y se mantiene en inclinación
de 35°, torsión de 25° y extensión de 20°.

También se corrigió un bug latente en `save_textures()`: derivaba el nombre de salida del
primer token del datablock, que para todo el set nuevo es `T`, así que **los ocho mapas se
escribían sobre un único archivo** y quedaban repuntados a él.

#### P7 — auditoría de deformación

Flexiones aplicadas y renderizadas: codo 95°, hombro 75°, cadera 85°, rodilla 105°.
Cambio de volumen de la malla evaluada entre −0,2% y −1,15%: sin colapso de articulación.
Adjunción del equipo verificada numéricamente — al rotar `lowerarm_l` sólo se mueve la
computadora de muñeca; al rotar `head` sólo el casco; ambos siguen a `spine_03`.

---

#### P4 — morphs faciales

Medición previa de la topología: **249 vértices** en toda la cara (arista media 8,9 mm),
~18 por ojo, 16 en la boca. El rig es un esqueleto UE5 estándar **sin huesos faciales**
(sólo `neck_01` y `head`), así que morph target es la única vía.

Anclajes obtenidos del render QA con cámara ortográfica, que da conversión exacta píxel → z
(0,34 m sobre 1100 px, centro en 1,715). La escala se validó contra la coronilla:
z = 1,8287 medido frente a 1,83 real. Líneas oscuras de la columna central:

| Rasgo | z |
|---|---|
| línea de ojos y ceja | 1,6966 |
| base de la nariz | 1,6430 |
| línea de la boca | 1,6195 |
| nacimiento del pelo | ~1,7650 |

La silueta de la malla por sí sola inducía a error: la franja estrecha de z 1,58–1,63 no es
el cuello sino el mentón y la mandíbula.

| Morph | Vértices | Desplazamiento máx. |
|---|---|---|
| `jaw_open` | 679 | 18,98 mm |
| `brow_raise` | 122 | 7,00 mm |
| `brow_furrow` | 121 | 4,72 mm |

`jaw_open` rota la mandíbula completa alrededor del plano bisagra–comisura. Una banda
horizontal en Z no servía: dejaba fuera la rama de la mandíbula y estiraba el mentón.

**Limitación conocida:** los labios están sellados y no hay geometría interior de boca, así
que ningún morph puede *abrir* la boca — `jaw_open` desciende la mandíbula pero los labios
no se separan. Por eso la amplitud se fijó en 6° en vez de 15°: a 15° la cara se deformaba.
`blink` no se creó: con ~18 vértices por ojo el resultado sería peor que no tenerlo.

#### P12 — equipo: sets de textura y sombreado

Los cinco materiales procedurales planos de cada pieza se hornearon a sets por región,
1024 px, con `Color`, `NormalGL`, `ORM` y `Emission`:

- `Helmet` ← `SK_Astraeon_Helmet`
- `Gear` ← `SK_Astraeon_Backpack` + `SK_Astraeon_WristComputer`

Como `Gear` comparte una imagen entre dos objetos, primero se repartieron sus UV en mitades
disjuntas del espacio 0–1; si no, el segundo horneado pisaba al primero. El ORM no se puede
hornear directo — Blender no tiene pase de metálico — así que se sustituye temporalmente la
superficie de cada material por una emisión de color (1, roughness, metallic) y se hornea
EMIT, lo que da el canal exacto.

Sombreado suave por ángulo a 35°, que es lo que quita el facetado de la cúpula del casco.

#### P6 — validación en Unreal

Cuatro corridas, cada una destapando un contrato que la sesión anterior había escrito por
adelantado sin llegar a ejecutarlo:

1. `ModuleNotFoundError: MainCharacterAppearance` — `-ExecutePythonScript` no añade la
   carpeta del script a `sys.path`.
2. `SkeletalMesh object has no attribute set_material` — en la API de UE 5.7 los slots viven
   en la propiedad `materials`, como structs `SkeletalMaterial`, y hay que reescribir el
   array entero porque lo que devuelve la propiedad son copias.
3. `Destination exists; refusing overwrite` — la corrida 2 alcanzó a guardar materiales y
   texturas antes de fallar. Se borró sólo `/Optimized`; los 47 assets del import original
   quedaron intactos.
4. **`passed: true`, `saved_packages: true`.**

Además se ajustaron dos exigencias del script a la realidad del asset: el umbral de morphs
pasó de `>= 7` a los tres autorizados, y la asignación de material pasó del slot 0 a todos
los slots de cada pieza.

Resultado final, en `/Game/Astraeon/Characters/Player/Optimized` (68 paquetes):

| Comprobación | Resultado |
|---|---|
| Dimensiones | 183,56 × 38,05 × **183,00 cm** |
| LOD | 4 |
| Huesos | **75**, raíz `root`, sockets de casco/mochila/herramienta presentes |
| Clips | **45**, duración, movimiento y costura de bucle verificados uno a uno |
| Morph targets | `jaw_open`, `brow_raise`, `brow_furrow` |
| Materiales | `M_Player_Character`, `_Suit`, `_Gear`, `_Helmet` |
| Equipo | casco, mochila y computadora, mismo esqueleto |

#### P9 — integración de gameplay

`AstraeonPlayerCharacter` ya usa el protagonista real como cuerpo de sombra. Verificado
leyendo el CDO en el editor (`ContentPipeline/reports/player_body_wiring.json`):

| Campo | Valor |
|---|---|
| malla | `/Game/Astraeon/Characters/Player/Optimized/SK_Astraeon_Player` |
| esqueleto | `SK_Astraeon_Player_Skeleton` |
| altura | **183,0 cm** |
| LOD | 4 |
| morph targets | `jaw_open`, `brow_raise`, `brow_furrow` |
| slots de material | `MAT_Player_Character`, `MAT_Player_Suit` |
| posición relativa Z | −96,0 (pies en el suelo de la cápsula) |
| yaw | −90° |
| `bOwnerNoSee` | true |

#### Por qué se eligió convivir con dos esqueletos

El primer intento fue unificar: apuntar el cuerpo al asset nuevo manteniendo el invariante
"cuerpo y manos comparten esqueleto". Rompió `Astraeon.Art.Character.FirstPersonRigIsWired`
y, al investigarlo, resultó que unificar no era un cambio de una línea sino una reescritura
de la animación.

La medición que lo decidió: en este rig **el eje que lleva el brazo al frente es Z**, no X.
Se comprobó rotando `upperarm_r` y leyendo la posición mundial de `hand_r`:

```
Z = +0.9  ->  mano en y = -0.413   (adelante)
Z = -0.9  ->  mano en y = +0.415   (atrás)
X = ±0.9  ->  la mano sube o baja por el costado, y apenas cambia
```

Los 45 clips existentes usan Z entre **0,13 y 0,46**, casi nada. Medido sobre clips ya
auditados: en `TwoHand_Idle` las manos quedan a **x = ±0,48 m**, medio metro de separación,
y en `Inspect` la mano sube a la altura del pecho pero igual de abierta. Es decir, todo el
set posa los brazos en cruz y no al frente.

Consecuencia: migrar el rig de primera persona al esqueleto del protagonista habría exigido
re-autorizar las poses de brazo de los 45 clips para que las manos cayeran delante de la
cámara — un rehacer de la animación, con riesgo de regresar clips ya auditados, y sin más
beneficio para el jugador que la ruta elegida.

Dos esqueletos es además la arquitectura habitual: brazos de primera persona por un lado,
cuerpo en tercera por otro. El cuerpo aquí sólo existe para proyectar la silueta.

#### Qué cubre ahora la prueba

`Astraeon.Art.Character.FirstPersonRigIsWired` cambió la igualdad de esqueletos por lo que
esa igualdad protegía de forma indirecta:

- cada uno de los nueve gestos resuelve sobre el esqueleto de las manos (ya estaba);
- la malla del cuerpo carga y está skinneada a un esqueleto con raíz `root`;
- la silueta mide entre 170 y 195 cm, que es lo que hace legible la escala en el suelo;
- la raíz se apoya en el suelo de la cápsula.

No se debilitó nada: la aserción vieja no era el requisito, era una forma indirecta de
comprobarlo. AGENTS.md §7 pide exactamente esto — explicar el cambio y reemplazar por una
verificación equivalente.


---

## 2. Pendiente

### Gestos autorizados sin exportar
Se crearon en Blender seis clips que faltaban —`Pulse`, `Drill`, `Hammer`, `Maul`, `Consume`,
`Present`— llevando el set de 45 a **51 acciones**, asentados sobre el suelo y sin mover
ninguno de los 45 ya auditados (levante 0,00 mm en todos ellos). **No están exportados ni
importados**: no hacían falta para la integración elegida, y subirlos obliga a extender
`animation_contract.json` y a repetir la validación de Unreal, que hoy fija 45 clips.
Sirven para tercera persona o cinemáticas cuando se los necesite.


### P10 · Documentación y commit
Falta actualizar `docs/COMPLETION_PASSPORT.md` (fases D y E siguen en *pending*),
`Docs/DEVELOPMENT_STATE.md`, `Docs/TEST_REPORT.md` y crear el commit atómico del lote.

---

## 3. Trabajo futuro acordado

Ambos puntos se evaluaron, se decidieron y se aplazaron a conciencia el 2026-09-08.

1. **Retopología densa de la cabeza** (~8–10k tris con bucles de ojo y boca, y geometría
   interior de boca). Habilitaría `blink` y una apertura de boca real. Reabre bake y LOD.
   Aporta realismo si alguna vez hay cámara sobre la cara.
2. **Rediseño del casco.** La forma sigue siendo simple y de bandas rectas; el visor no
   envuelve y queda un hueco entre el borde inferior y el cuello. El sombreado y las
   texturas ya están resueltos, así que esto es puramente trabajo de forma.

---

## 4. Limitaciones conocidas

- Muescas oscuras en la silueta del pelo: pérdida de fidelidad de la retopo frente al arte
  original de 233k. `QA_Compare_Source_Face.png`.
- `jaw_open` desciende la mandíbula pero no separa los labios: la boca no tiene interior.
- Sin `blink`: densidad insuficiente alrededor del ojo.
- 8 vértices de las almohadillas de la mochila entran ≤ 7,4 mm en el traje; queda oculto.
