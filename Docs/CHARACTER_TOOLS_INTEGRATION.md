# Personaje y herramientas — integración jugable

2026-09-06. Cierra la brecha entre los lotes de Blender ya validados y el juego: el lote
humano y el de herramientas dejan de ser FBX sueltos y pasan a ser paquetes de `Content/`
montados sobre `AAstraeonPlayerCharacter`. Calidad **Q1**, sin arte final.

## Qué cambió

### 1. Desbloqueo del lote de herramientas

`ContentPipeline/reports/tools_unreal_import.json` estaba en `passed: false` con el error
`Grip-origin bounds mismatch`, y el lote quedó sin manifiesto ni integración.

La causa no era el arte: `Scripts/Editor/ValidateToolsImport.py` comparaba el origen de los
bounds de Unreal contra el centro medido en Blender **sin aplicar el espejo en Y** que
introduce `convert_scene` al pasar del marco diestro de Blender al de Unreal. La comprobación
de dimensiones pasaba porque las dimensiones son absolutas; la de origen no, porque el signo
de Y sí importa.

Medido tras el fix, para el escáner: `origin_cm.y = +3.0` frente a un centro Blender de
`-3.0`. El chequeo ahora niega Y y además registra `origin_cm`/`expected_origin_cm` por
asset, de modo que un fallo futuro traiga los números en vez de sólo el mensaje.

Resultado: 8 mallas y 7 animaciones aprobadas, exit 0, commandlet 0 errores / 0 warnings, y
`ContentPipeline/tools_asset_manifest.json` generado (15 exports).

### 2. Paquetes persistentes en `Content/`

Dos scripts nuevos, con el mismo patrón fail-closed que `PrepareRegionBlockoutAssets.py`
(verifican hashes de config/generador/validador y de cada FBX, hacen preflight de todo
destino antes de importar o guardar nada, y comparan el tag `AstraeonSourceHash` de lo que
ya exista antes de reimportar):

- `Scripts/Editor/PrepareHumanoidBlockoutAssets.py` → `/Game/Astraeon/Art/Blockouts/Human`
  (14 paquetes: `SKEL_Humanoid_A`, dos SkeletalMesh, siete AnimSequence, cuatro materiales).
- `Scripts/Editor/PrepareToolsBlockoutAssets.py` → `/Game/Astraeon/Art/Blockouts/Tools`
  (21 paquetes: ocho StaticMesh, siete AnimSequence, seis materiales).

Los materiales planos se crean por **nombre de slot**, que viaja en el FBX desde el nombre
del material de Blender (`M_Human_fabric`, `M_Tool_Grip`…). Un slot que no esté en la paleta
del config aborta el script en vez de dejar el gris por defecto del motor.

El esqueleto se renombra a `SKEL_Humanoid_A` antes de guardar, para no arrastrar el
`SK_Human_Body_Blockout_Skeleton` que genera el importador ni dejar un redirector.

Verificado en el guardado: cuerpo de 180,0 cm exactos, 57 huesos, ambas mallas sobre el
mismo esqueleto, duración de cada clip igual a `(frames-1)/fps`.

### 3. Montaje sobre el Character

`UAstraeonFirstPersonRigComponent` (`Source/Astraeon/{Public,Private}/Presentation/`) es la
malla de manos y cuelga de `FirstPersonCamera`, así que sigue el cabeceo de la vista.
De ella cuelga la herramienta, atada al hueso `socket_tool_r`: la sujeción viene de la
animación, no de una posición fija en pantalla. Del taladro cuelga además la broca, que
sólo gira mientras el taladro está en la mano.

La selección de clip es un estado pequeño en C++ sobre `PlayAnimation`, **no un
AnimBlueprint**: es Q1 y evita depender de un asset de Blueprint que ningún generador
reproduce. No hay BlendSpace todavía; los cambios locomotores aplican histeresis y conservan
la fase normalizada para no reiniciar el ciclo visiblemente.

- Locomoción: caer → `Jump`; recién aterrizado (0,6 s) → `Land`; ≥620 cm/s → `Run`;
  ≥16 cm/s → `Walk`; al desacelerar, `Run` se conserva hasta 560 cm/s y `Walk` hasta 6 cm/s;
  si no → `Idle`.
- Gestos puntuales, que interrumpen la locomoción durante la duración del clip:

  | Acción | Gesto |
  |---|---|
  | Escanear (click izq.) | `Scan` |
  | Colocar pieza (click izq. en modo construcción) | `Hammer` |
  | Disparar la cortadora (click der.) | `Pulse` |
  | Demoler (click der. en modo construcción) | `Maul` |
  | `E` con el taladro en mano | `Drill` |
  | `E` en cualquier otro caso | `Interact` |
  | Usar ración / resonador / otro ítem | `Consume` / `Present` / `Grip` |

La herramienta visible se elige por `GetHandItemId()`. Con las manos vacías se muestra el
escáner, que el diseño da por disponible desde el primer minuto y no ocupa ranura de
inventario. **Ningún `FName` de ítem, receta o marcador cambió**: la presentación se cuelga
de los ids, no al revés.

El cuerpo completo (`GetMesh()`) existe sólo para la sombra propia: `bOwnerNoSee` más
`bCastHiddenShadow`, raíz a -96 cm (media altura de cápsula) y yaw -90 (el frente del rig
queda en +Y tras la importación).

El `Tick` del Character pasó de 0,2 s a por frame para que el rig pueda cambiar de clip sin
200 ms de retraso; la lógica de juego que corría a 5 Hz se sigue evaluando a 5 Hz mediante
un acumulador, así que su cadencia no cambió.

### 4. Dos tests obsoletos, corregidos

`Astraeon.WorldGen.Region.Invariants` y `Astraeon.Art.Region.PresentationPreservesGameplay`
fallaban **antes** de este trabajo, por la fase de mundo vivo: la región pasó de un punto de
aparición de criatura a cinco (`AstraeonScience::CreatureSpawnCount = 5`), así que el conteo
fijo de tres POIs y el descarte por el id antiguo `first_mob_patrol_origin` quedaron
obsoletos. Se reemplazó el conteo mágico por una comprobación de composición (una señal, una
anomalía, el resto puntos de aparición, y nada fuera de esos tres tipos) y el descarte por
prefijo `creature_spawn`. No se tocó gameplay.

## Verificación

- `AstraeonEditor Win64 Development`: compilación exitosa, 0 warnings.
- Automation `Astraeon.*`: **49 encontrados, 49 exitosos**, exit 0.
- Smoke crítico editor-game:
  `AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- Test nuevo `Astraeon.Art.Character.FirstPersonRigIsWired`: comprueba que el rig cuelga de
  la cámara, que las manos cargan desde `Content` con 57 huesos y raíz `root`, que el cuerpo
  comparte esqueleto y no lo ve su dueño pero sí proyecta sombra, que los nueve gestos
  resuelven a clips sobre ese mismo esqueleto, que la herramienta cuelga de `socket_tool_r`
  sin colisión, y que los cinco clips de locomoción y las ocho mallas de herramienta cargan
  por ruta. Si alguien mueve o renombra un paquete, esto lo detecta antes que una sesión manual.

Evidencia: `ContentPipeline/reports/humanoid_content_packages.json`,
`tools_content_packages.json`, `tools_unreal_import.json`,
`ContentPipeline/tools_asset_manifest.json`.

## Pendiente de revisión humana

Nada de lo siguiente se puede certificar sin jugar: son ajustes de encuadre, no de contrato.

1. **Encuadre de las manos.** `HandsOffsetCm` (0, 0, -160) y `HandsRotation` (0, -90, 0)
   están derivados de la geometría (ojo del rig a 1,60 m sobre la raíz), no medidos en
   cámara. Ambos son `EditAnywhere` en el componente para ajustarlos sin recompilar.
   En pose A los brazos cuelgan a los lados: es esperable ver poco las manos en `Idle` y
   `Walk`, y verlas bien en los gestos, que levantan el brazo.
2. **Pose de agarre.** `ToolGripTransform` se derivó de `attachment.matrix_local_blender_m`
   conjugando por C = diag(1,-1,1) y transponiendo para el convenio de vectores fila de
   Unreal. La matemática cierra (ortonormal, det +1) pero **no está confirmada visualmente**:
   si la herramienta aparece girada o desplazada en la mano, ese transform es el sospechoso.
3. **Eje de giro de la broca.** Se asume el pitch de Unreal (Y de Blender espejado).
4. Deslizamiento de pies, ausencia de IK y corte entre clips: límites Q1 ya registrados en
   `HUMANOID_ART.md`.

## Lo que este trabajo no hace

No hay AnimBlueprint, BlendSpace, montage ni capas. No hay VFX ni audio asociados a los
gestos. Las herramientas no tienen colisión ni física. El cuerpo no tiene PhysicsAsset. No
se tocó la nave, la criatura, el terreno ni la UI. No se empaquetó una build nueva.
