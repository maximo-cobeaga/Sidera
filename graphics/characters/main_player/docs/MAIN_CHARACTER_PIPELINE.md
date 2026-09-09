# ASTRAEON — Main Character Pipeline

Fecha: 2026-09-08. Estado: **TERMINADO E INTEGRADO — optimizado, horneado, validado en
Unreal 5.7.4 y conectado a `AstraeonPlayerCharacter`.**

Cifras vigentes (las del cuerpo del documento son de la fase de blockout, 2026-09-07):

| | 2026-09-07 | 2026-09-08 |
|---|---|---|
| LOD0 | 233.846 tris | **65.284** (100% quads) |
| LOD1/2/3 | 93.538 / 37.414 / 14.030 | **32.642 / 16.321 / 6.527** |
| huesos | 71 | **75** (4 twist aditivos) |
| texturas | atlas único | **Character, Suit, Gear, Helmet** separados |
| morph targets | 0 | **3** (jaw_open, brow_raise, brow_furrow) |
| equipo | casco blockout, sin UV | casco/mochila/computadora con UV y sombreado suave |
| acciones en Blender | 45 | **51** (6 gestos sin exportar) |

Estado, decisiones y limitaciones al día: [PENDIENTE_PROTAGONISTA.md](../../../../Docs/PENDIENTE_PROTAGONISTA.md).

Fuente Blender: `blender/CHR_Astraeon_Player.blend` (canónico, texturas empaquetadas, 113 MB).
Los checkpoints `CHK_*.blend` de la sesión se eliminaron tras verificar el resultado y están
ignorados por git: son copias de recuperación de ~100 MB cada una y `.blend` va a LFS.
Generadores: `Tools/Blender/main_character_{rig,anim,helmet,export}.py`.

---

## Character

Explorador científico e ingeniero de campo, masculino, ~30 años, 1,83 m, atlético funcional.
Traje ASTRAEON en blanco técnico + azul marino/grafito, con acentos cian funcionales en rodillas
y placas ligeras en pecho, hombros, antebrazos, muslos y espinillas. Cabello castaño oscuro corto
y barba muy corta. Rostro visible; el casco es un objeto independiente y desmontable.

Origen: generación Tripo `8a550175-48f6-4785-939c-f45ebb02501c` (10 créditos, `geometry_quality=detailed`,
`texture_quality=standard`, PBR). Fuente preservada: `blender/SOURCE_Tripo_8a550175.glb`
(56 MB, SHA256 `6a28d33c…2956e`, 1.948.729 tris) y `SRC_Player_MESH_01` en la escena.

---

## Architecture

| Objeto | Colección | Tris | Rol |
| --- | --- | --- | --- |
| `SK_Astraeon_Player` | ASTRAEON_BODY | 233.846 | LOD0, malla de juego |
| `SK_Astraeon_Player_LOD1` | ASTRAEON_BODY | 93.538 | LOD1 |
| `SK_Astraeon_Player_LOD2` | ASTRAEON_BODY | 37.414 | LOD2 |
| `SK_Astraeon_Player_LOD3` | ASTRAEON_BODY | 14.030 | LOD3 |
| `SK_Astraeon_Helmet` | ASTRAEON_HELMET | 2.764 | Casco desmontable (blockout) |
| `SKEL_Astraeon_Player` | ASTRAEON_RIG | — | Armature, 71 huesos |
| `SRC_Player_MESH_01` | ASTRAEON_GENERATED_SOURCE | 1.948.729 | Fuente de detalle, oculta |

Escena `CHR_Astraeon_Player_Work`, métrica, escala 1, 30 FPS. Los 23 objetos `PROXY_*` del
blockout de proporciones se conservan ocultos como referencia histórica.

### Orientación y escala

El personaje mira a **-Y**; **+X es su lado izquierdo** (`_l`). Altura medida 1,830 m,
envergadura 1,837 m, pies en Z = 0, transformaciones aplicadas (loc 0, rot 0, escala 1).

> El importador glTF deja los objetos en `rotation_mode='QUATERNION'`. La sesión anterior
> escribió `rotation_euler.z` sobre ese objeto y la rotación **nunca se aplicó**: el personaje
> quedó mirando a -X y el render llamado «Front» mostraba en realidad el perfil.

---

## Rig

`SKEL_Astraeon_Player`: 71 huesos, 52 deformantes, raíz única `root`. Nomenclatura compatible
con el esqueleto humano estándar de Unreal.

```
root
└── pelvis → spine_01 → spine_02 → spine_03 → neck_01 → head
    ├── clavicle_[lr] → upperarm → lowerarm → hand
    │   └── thumb_01..03, index_01..03, middle_01..03, ring_01..03, pinky_01..03
    └── thigh_[lr] → calf → foot → ball
ik_foot_root → ik_foot_[lr]
ik_hand_root → ik_hand_gun → ik_hand_[lr]
```

Todas las posiciones provienen de medición sobre la malla (secciones por Z/X, centroides de
sección, y curvas mediales de los dedos obtenidas por clustering radial 3D). Longitudes
resultantes: brazo 301 mm, antebrazo 250 mm, muslo 435 mm, pierna 402 mm, falanges 38/23/24 mm.
Simetría izquierda/derecha exacta.

**Sin twist bones en esta versión.** Se omitieron para no fragmentar el reparto de pesos;
Unreal los deriva o se añaden en una pasada posterior.

### Convención de ejes (medida, no asumida)

Los ejes locales de los huesos laterales están espejados, por lo que el mismo valor numérico
produce movimientos opuestos según el eje. Tabla verificada empíricamente:

| Hueso | `+X` | `+Z` |
| --- | --- | --- |
| `upperarm_[lr]` | brazo arriba | `_l` atrás / `_r` adelante |
| `lowerarm_[lr]` | flexión de codo | `_l` atrás / `_r` adelante |
| `thigh_[lr]` | flexión de cadera (pierna adelante) | abducción |
| `calf_[lr]` | extensión de rodilla (**flexión es `-X`**) | — |
| `foot_[lr]` | punta arriba | — |
| dedos | cierre hacia la palma | separación |
| `spine_*` | inclinación adelante | torsión (cabeza hacia -X) |
| `neck_01`, `head` | inclinación adelante | giro a la derecha del personaje |

Reflejar una pose = intercambiar `_l`/`_r`, conservar X, negar Y y Z.

---

## Skinning

**El bone heat weighting de Blender falla por completo en esta malla** (la salida de Tripo tiene
47.871 vértices de borde abierto y shells sueltos): crea los 52 grupos vacíos y deja los 140.653
vértices sin peso, con el aviso «Falla al buscar solución para uno o más huesos».

Sustituido por un solver propio, en `bl_execute` sobre NumPy:

1. distancia punto–segmento de cada vértice a los 52 huesos deformantes;
2. filtro de lateralidad — un vértice con `x < -0.02` no recibe influencia de huesos `_l`
   ni al revés, lo que evita el sangrado entre piernas y entre brazo y torso;
3. los 4 huesos más cercanos, peso `1/(d+ε)^4`, normalizado;
4. escritura en grupos por cubos de 1/128 (equivalente a la cuantización a 8 bits de Unreal).

Resultado: **0 vértices sin peso**, los 52 huesos con influencia, distancia media al hueso más
cercano 87 mm. Validado en pose extrema (brazos abajo, codos flexionados, puño cerrado, zancada
con rodilla flexionada, cabeza girada): ver `references/QA_Pose_{Front,Side}.png`.

---

## Attachments

Huesos no deformantes, listos como sockets en Unreal:

| Hueso | Padre | Uso |
| --- | --- | --- |
| `weapon_[lr]`, `tool_[lr]` | `hand_[lr]` | arma / herramienta en mano |
| `socket_tool_[lr]` | `hand_[lr]` | alias que consume el runtime actual |
| `back_attach`, `socket_backpack` | `spine_03` | mochila y módulos traseros |
| `hip_[lr]_attach` | `pelvis` | fundas de cadera |
| `socket_helmet` | `head` | montaje del casco |
| `ik_hand_[lr]`, `ik_hand_gun`, `ik_foot_[lr]` | cadenas IK | jerarquía IK estándar de UE |

El grip se sitúa al 55 % del hueso `hand`, es decir el centro medido de la palma.

---

## Helmet

`SK_Astraeon_Helmet`, objeto independiente, 2.764 caras, ponderado 100 % al hueso `head`,
por lo que Unreal puede alternar su visibilidad sin tocar la cabeza. Cuatro slots de material:
`MAT_Player_Helmet{Shell,Trim,Glass,Emission}`. Elipsoide ahusado con abertura frontal cortada
por bisect de tres planos, visor hundido 4,5 %, marco de trim de 13 mm y dos tiras de emisión cian.

> **Fidelidad: blockout.** La estructura, la escala (15–20 mm de holgura sobre la cabeza medida),
> los materiales y el montaje son correctos, pero la silueta todavía lee como una cúpula lisa y el
> borde lateral de la abertura conserva escalones de una cara. **No es el casco AAA del brief** y
> requiere una pasada de modelado dedicada. Ver `references/QA_Helmet.png`.

> Nota de plataforma: esta instalación de Blender está localizada en español y los nodos de
> material se crean con nombre traducido (`BSDF Principista`). `nodes.get('Principled BSDF')`
> devuelve `None` y descarta todos los ajustes **en silencio**. Buscar siempre por
> `node.type == 'BSDF_PRINCIPLED'` y las entradas por `socket.identifier`.

---

## Materials

Cuerpo: **un solo material**, `MAT_Astraeon_Player_Atlas`, con un único `UVMap` y tres texturas
de 2048² empaquetadas y exportadas a `textures/`:

| Archivo | Espacio de color | Contenido |
| --- | --- | --- |
| `T_Astraeon_Player_Color.png` | sRGB | Base Color |
| `T_Astraeon_Player_NormalGL.png` | Non-Color | Normal (OpenGL, invertir verde en UE) |
| `T_Astraeon_Player_ORM.png` | Non-Color | AO / Roughness / Metallic en R/G/B |

**Los sets de textura Character / Suit / Gear / Helmet no están separados**: la generación
entrega un atlas fusionado y separarlos exige reasignar índices de material por región y
rehornear. Pendiente.

---

## Animations

45 Actions a 30 FPS, todas **in-place**, generadas por `Tools/Blender/main_character_anim.py`.
Datos verificados por clip en `animations/animation_contract.json`.

| Grupo | Clips | Frames |
| --- | --- | --- |
| Idle | `AN_Player_Idle` | 91 |
| Marcha | `AN_Player_Walk_{F,B,L,R}` | 37 |
| Carrera | `AN_Player_Run_{F,B,L,R}` | 25 |
| Salto | `AN_Player_Jump_{Start,Loop,Land}` | 16 / 31 / 22 |
| Agachado | `AN_Player_Crouch_{Enter,Idle,Walk_F,Walk_B,Walk_L,Walk_R,Exit}` | 22 / 61 / 43 / 22 |
| Interacción | `AN_Player_{Interact,Pickup,UseTool,Scan,Inspect}` | 46 / 61 / 46 / 76 / 91 |
| Una mano | `AN_Player_OneHand_{Equip,Idle,Aim,Lower,Unequip}` | 31 / 61 / 31 |
| Dos manos | `AN_Player_TwoHand_{Equip,Idle,Aim,Lower,Unequip}` | 31 / 61 / 31 |
| Herramienta | `AN_Player_Tool_{Equip,Idle,Use,Unequip}` | 46 / 61 |
| Agarres | `Grip_{Open,Fist,Pistol,Rifle,Tool,Object,TwoHand}` | 1 |

Marcha y carrera se construyen desde un medio ciclo de 4 claves más su espejo; el paso lateral
tiene 8 claves propias porque no es simétrico. Marcha atrás = ciclo invertido conservando la fase 0.

### Contacto con el suelo

Sondas de suela sobre `ball`/`foot` (punta, talón y centro de planta) evaluadas fotograma a
fotograma. La altura de pelvis se corrige **solo hacia arriba**, de modo que se conserva la fase
aérea de carrera y salto. Dos pasadas: por clave y, donde la interpolación Bézier seguía hundiendo
el pie entre claves, horneado por fotograma.

| Métrica | Antes | Después |
| --- | --- | --- |
| Peor penetración | −78,8 mm (`Jump_Start`) | **−1,2 mm** |
| Costura de loop (20 clips cíclicos) | 0 mm | 0 mm |

Evidencia visual: `references/QA_Locomotion_Sheet.png` (marcha y carrera, 4 fotogramas cada una).

---

## Grip System

Las 7 poses `Grip_*` son Actions de un fotograma sobre la pose base, reutilizables como aditivas
o como poses de referencia en Unreal. Mano derecha dominante; la izquierda se resuelve por IK
contra `ik_hand_l`. Los props no forman parte de la malla del personaje: se adjuntan a
`weapon_*` / `tool_*`.

---

## Unreal Import

Unreal Engine **5.7.4**, fijado. Contrato de export (`Tools/Blender/main_character_export.py`),
alineado con `Tools/Blender/exporters/fbx.py`:

```
global_scale=1.0, apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE'
axis_forward='-Y', axis_up='Z', add_leaf_bones=False
primary_bone_axis='Y', secondary_bone_axis='X', mesh_smooth_type='FACE'
```

El armature se renombra temporalmente a `Armature` durante el export para que Unreal descarte el
contenedor y `root` quede como raíz real del Skeleton (necesario para root motion futuro).

| Archivo en `exports/` | Contenido |
| --- | --- |
| `SK_Astraeon_Player.fbx` | Malla LOD0 + esqueleto, sin animación |
| `SK_Astraeon_Player_LOD{1,2,3}.fbx` | Niveles de detalle, mismo esqueleto |
| `SK_Astraeon_Helmet.fbx` | Casco, mismo esqueleto |
| `AN_Astraeon_Player_All.fbx` | 45 pistas, una por Action |

Procedimiento: importar `SK_Astraeon_Player.fbx` con **Import Uniform Scale 1** y sin
«Convert Scene Unit»; asignar los LOD manualmente; importar las animaciones contra el Skeleton
resultante. Reconstruir el material PBR en Unreal a partir de los tres PNG (**invertir el canal
verde de la normal**: la textura es OpenGL). El casco se añade como componente aparte enlazado a
`socket_helmet`.

### Validación de export ejecutada

Round-trip real: los FBX se reimportaron en una escena aislada de Blender.

| Comprobación | Resultado |
| --- | --- |
| Altura | 1,8300 m |
| Pies en el origen | Z mín = 0,0000 |
| Envergadura / profundidad | 1,8372 m / 0,3815 m |
| Huesos, raíz | 71, raíz única `root` |
| Triángulos LOD0 | 233.846 |
| Grupos de vértices / sin peso | 52 / **0** |
| Escala del objeto | 1, 1, 1 |
| Pistas de animación | **45**, a 30 FPS |

> Bug detectado y corregido en esta pasada: quitar la Action **no** restaura los pose bones, por lo
> que el primer FBX salió con el esqueleto posado (envergadura 1,323 m) y Unreal habría tomado esa
> pose como reference pose del Skeleton. `main_character_export.reset_pose()` lo previene.

**No se ha ejecutado una importación real en el editor de Unreal** en esta sesión: la validación
es del FBX, no del asset de UE.

---

## LOD

| Nivel | Tris | Ratio sobre LOD0 |
| --- | --- | --- |
| LOD0 | 233.846 | 1,00 |
| LOD1 | 93.538 | 0,40 |
| LOD2 | 37.414 | 0,16 |
| LOD3 | 14.030 | 0,06 |

Todos conservan los 52 grupos de vértices con 0 vértices sin peso, el `UVMap` y el material.

LOD0 es denso para un personaje de juego porque la malla **no está retopologizada**: es la salida
generada decimada al 12 %. Se calibró contra el rostro: por debajo de ~0,10 aparecen bandas planas
en frente y pómulos. La marca diagonal de la mejilla **no es de la decimación** — está presente en
la fuente de 1.948.729 tris y es una costura de textura de Tripo.

---

## Known Limitations

1. **Sin retopología.** LOD0 es decimación de una malla generada: sin edge loops orientados a las
   articulaciones y con densidad uniforme. Una pasada de retopo + bake de normales bajaría LOD0 a
   60–80 k tris con mejor deformación. Es la mejora de mayor impacto pendiente.
2. **Casco blockout**, no arte final (ver sección Helmet).
3. **Un solo material y un solo set de textura** para todo el cuerpo; sin separación
   Character / Suit / Gear.
4. **Sin shape keys ni expresiones faciales.** No se creó ninguna base facial.
5. **Sin twist bones.**
6. **Sin mochila ni computadora de muñeca**: se excluyeron deliberadamente del prompt de
   generación para no comprometer la anatomía; deben modelarse como props adjuntos.
7. Skinning por distancia: correcto en las pruebas realizadas, pero sin weight painting manual
   en axilas, ingle y hombros.
8. La costura de textura en la mejilla derecha viene de la generación y no se ha retocado.
9. **Sin compilación, importación en Unreal, smoke test ni build nuevo** en este lote.
10. Créditos Higgsfield: el saldo era 10 (plan free) y la generación consumió 10. Cualquier
    generación adicional requiere recargar.

---

## Future Work

1. Retopología manual/semiautomática + bake de Normal/AO desde `SRC_Player_MESH_01`; rehacer los LOD.
2. Pasada de modelado del casco: perfil facetado, visor envolvente, mentonera, acentos bronce.
3. Separar índices de material por región y hornear sets Character / Suit / Gear / Helmet.
4. Weight painting manual en hombros, axilas, ingle y muñecas; añadir twist bones.
5. Shape keys faciales mínimas: blink L/R, mouth open, smile, frown, brows up/down.
6. Modelar mochila y computadora de muñeca como props sobre `socket_backpack` y `lowerarm_l`.
7. Ensayo de importación real en Unreal 5.7.4 en destino aislado; medir altura, jerarquía, skin,
   normales y duraciones antes de conectar el runtime.
8. Root motion: variantes con desplazamiento para los clips de locomoción, usando
   `travel_speed_ms` del contrato como referencia.
