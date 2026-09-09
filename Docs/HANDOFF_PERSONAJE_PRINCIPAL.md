> **SUPERADO — 2026-09-08.** Este handoff describía el estado al cerrar el pipeline de
> Blender. Desde entonces el protagonista se optimizó a 65.284 triángulos, se rehornearon
> sus texturas, se regeneraron los LOD, se le añadieron tres morph targets, se validó en
> Unreal 5.7.4 y **quedó integrado en el juego** como cuerpo de sombra de
> `AstraeonPlayerCharacter`.
>
> Estado vigente, decisiones y limitaciones: [PENDIENTE_PROTAGONISTA.md](PENDIENTE_PROTAGONISTA.md).
> Lo de abajo se conserva como registro de aquella sesión; sus cifras (233.846 tris, 71
> huesos, casco blockout, texturas sin separar) ya no son el estado actual.

---

# Handoff — Personaje principal (reanudar 2026-09-08)

Sesión cerrada el 2026-09-07. El pipeline Blender del protagonista está **cerrado y verificado**;
lo que falta es la integración en Unreal y tres pasadas de calidad artística.

Referencia completa: [MAIN_CHARACTER_PIPELINE.md](../graphics/characters/main_player/docs/MAIN_CHARACTER_PIPELINE.md).

---

## 1. Estado en una línea

Personaje jugable **game-ready en Blender**: malla 233.846 tris a 1,83 m, rig de 71 huesos con
nomenclatura Unreal, skinning sin huecos, 45 animaciones auditadas y FBX validados por round-trip.
**Nunca se ha importado en el editor de Unreal.**

---

## 2. Qué abrir mañana

```
graphics/characters/main_player/blender/CHR_Astraeon_Player.blend
```

Archivo canónico, 113 MB, texturas empaquetadas, escena `CHR_Astraeon_Player_Work`, 30 FPS,
rig en reposo, fotograma 1, cámara `CAM_Player_Front` activa.

Para reconectar el Bridge: abrir Blender → panel Higgsfield → **Connect**. Verificar con
`get_host_status` que devuelva `blr: true`.

---

## 3. Inventario en disco (220 MB)

| Ruta | Contenido |
| --- | --- |
| `blender/CHR_Astraeon_Player.blend` | Fuente canónica |
| `blender/SOURCE_Tripo_8a550175.glb` | Malla generada original, 1.948.729 tris — **no borrar**, es la fuente para el bake de normales |
| `exports/SK_Astraeon_Player.fbx` | LOD0 + esqueleto |
| `exports/SK_Astraeon_Player_LOD{1,2,3}.fbx` | LOD |
| `exports/SK_Astraeon_Helmet.fbx` | Casco (blockout) |
| `exports/AN_Astraeon_Player_All.fbx` | 45 pistas de animación |
| `textures/T_Astraeon_Player_{Color,NormalGL,ORM}.png` | 2048², PBR |
| `animations/animation_contract.json` | Datos verificados por clip |
| `references/*.png` | Evidencia visual de QA |

Generadores reproducibles: `Tools/Blender/main_character_{rig,anim,helmet,export}.py`.

Los checkpoints `CHK_*.blend` de la sesión se eliminaron (663 MB liberados) y quedaron ignorados
en `.gitignore`; `.blend` va a LFS y no tiene sentido versionar copias intermedias.

---

## 4. Orden sugerido para mañana

### Paso 1 — Importar en Unreal 5.7.4 *(bloqueante para todo lo demás)*

Es lo único que separa este trabajo de ser usable en el juego, y es donde aparecen las sorpresas.

1. Destino **aislado**, no reemplazar los paquetes Q1: `Content/Astraeon/Characters/Player/`.
2. Importar `SK_Astraeon_Player.fbx` con **Import Uniform Scale = 1**, sin «Convert Scene Unit»,
   Skeleton = nuevo.
3. Verificar en el editor: altura 183 cm, `root` como hueso raíz, 71 huesos, sin escala residual,
   reference pose en T (envergadura ~184 cm, **no** brazos abajo).
4. Importar `AN_Astraeon_Player_All.fbx` contra ese Skeleton → deben aparecer 45 AnimSequences a 30 FPS.
5. Reproducir `AN_Player_Walk_F` y `AN_Player_Run_F` en el visor y comprobar contacto de pies.
6. Reconstruir el material desde los tres PNG. **Invertir el canal verde de la normal**: la textura
   es OpenGL y Unreal espera DirectX.
7. Asignar LOD1–3 manualmente al Skeletal Mesh.
8. Añadir el casco como componente aparte enlazado a `socket_helmet`.

Sockets que espera el runtime actual: `socket_tool_r` existe en el rig; verificar que
`AAstraeonPlayerCharacter` lo encuentre antes de conectar nada.

### Paso 2 — Retopología + bake *(mayor impacto de calidad)*

LOD0 son 233 k tris porque es decimación de una malla generada, sin edge loops orientados a las
articulaciones. Retopo a 60–80 k tris + bake de Normal/AO desde `SOURCE_Tripo_8a550175.glb`
mejoraría simultáneamente rendimiento y deformación. Después rehacer LOD1–3 y re-exportar.

### Paso 3 — Casco

Lo entregado es **blockout**: estructura, escala, materiales y montaje correctos, silueta no.
Pendiente: perfil facetado en vez de cúpula lisa, visor envolvente con mentonera, acentos bronce,
y resolver los escalones de una cara en el borde lateral de la abertura (el corte por bisect
resolvió los bordes horizontales, no los laterales).

### Paso 4 — Resto de calidad

- Separar índices de material por región y hornear sets Character / Suit / Gear / Helmet.
- Weight painting manual en hombros, axilas, ingle y muñecas; añadir twist bones.
- Shape keys faciales mínimas: blink L/R, mouth open, smile, frown, brows up/down.
- Mochila y computadora de muñeca como props sobre `socket_backpack` y `lowerarm_l`.
- Variantes con root motion para locomoción, usando `travel_speed_ms` del contrato.

---

## 5. Trampas verificadas — no volver a pisarlas

1. **Blender en español.** Los nodos se crean con nombre traducido (`BSDF Principista`).
   `nodes.get('Principled BSDF')` devuelve `None` y **descarta los ajustes sin error**.
   Buscar por `node.type == 'BSDF_PRINCIPLED'` y las entradas por `socket.identifier`.
2. **El importador glTF deja `rotation_mode='QUATERNION'`.** Escribir `rotation_euler` sobre esos
   objetos no hace nada. Esto ya causó una orientación equivocada silenciosa.
3. **Quitar la Action no restaura los pose bones.** Exportar sin llamar antes a
   `main_character_export.reset_pose()` produce un FBX posado, y Unreal toma esa pose como
   reference pose del Skeleton.
4. **El bone heat weighting falla en esta malla** (47.871 vértices de borde abierto): crea los
   grupos vacíos y avisa, pero no falla. Si hay que re-skinear, usar el solver por distancia
   documentado en la sección Skinning del pipeline.
5. **Ejes locales espejados.** El mismo valor numérico mueve `_l` y `_r` en sentidos opuestos según
   el eje. La tabla verificada está en el pipeline; reflejar = intercambiar lado, conservar X, negar Y y Z.
6. **La marca diagonal de la mejilla derecha no es de la decimación**: está en la fuente de
   1.948.729 tris, es una costura de textura de Tripo. No perder tiempo bajando el ratio.
7. **El vertex group de Decimate no sirve para preservar zonas.** Con un grupo asignado el
   modificador ignora el `ratio` y se queda en el 82 % de las caras, sea cual sea el factor.
8. **Blender 5.x usa Actions con slots**: `Action.fcurves` ya no existe. Las curvas están en
   `act.layers[0].strips[0].channelbag(slot).fcurves` (helper `action_fcurves()` en el módulo de animación).

---

## 6. Presupuesto

Créditos Higgsfield: saldo era **10** (plan free) y la generación Tripo los consumió. **Quedan 0.**
Ninguna de las tareas de arriba necesita generación; todo se hace con `bl_execute` sobre Blender.
Si mañana se quiere otra generación, hay que recargar primero.

---

## 7. Sin tocar en esta sesión

No hubo compilación de C++, ni empaquetado, ni smoke test, ni cambios en `Source/`.
Las prioridades 1–4 del bloque anterior de `DEVELOPMENT_STATE.md` (bug de la ESCOTILLA con la
tecla `E` y el smoke manual posterior) **siguen abiertas y sin tocar**.
