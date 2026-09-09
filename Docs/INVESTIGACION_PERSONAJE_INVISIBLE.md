# Personaje invisible en juego — investigación

Fecha: 2026-09-08. Estado: **SIN RESOLVER**.

Síntoma reportado por el propietario: el protagonista no se ve. Ni como sombra en primera
persona, ni con la cámara en tercera persona (tecla V). La cámara alterna correctamente,
pero donde debería estar el personaje no hay nada.

Este documento existe porque el problema **no se resolvió** y la próxima sesión no debería
repetir lo ya descartado.

---

## 1. Lo que sí quedó hecho y funciona

| | Evidencia |
|---|---|
| Cámara en tercera persona con tecla **V** | `Astraeon.Art.Character.FirstPersonRigIsWired` alterna y verifica en ambos sentidos |
| El personaje está asignado al `AstraeonPlayerCharacter` | `ContentPipeline/reports/player_body_wiring.json` |
| El cuerpo recibe animación de su propio esqueleto | log: `animacion=AN_Astraeon_Player_All_Armature_AN_Player_Idle` |
| Build, 55 pruebas automáticas y smoke crítico | `Docs/TEST_REPORT.md` |

Herramienta de diagnóstico añadida y reutilizable, en `AstraeonPlayerCharacter`:

| Parámetro de línea de comandos | Efecto |
|---|---|
| `-AstraeonStartThirdPerson` | arranca en tercera persona sin depender de pulsar V |
| `-AstraeonCameraShot` | inicia partida y saca una captura a los 12 s |
| `-AstraeonBasicBodyMaterial` | fuerza `WorldGridMaterial` en el cuerpo (A/B de material) |
| consola `Astraeon.ToggleCamera` | alterna la vista desde `-ExecCmds` |

`LogCameraState()` vuelca en cada cambio de vista: brazo pedido vs real, malla, banderas de
visibilidad, posiciones de cuerpo y cámara, materiales por slot y animación activa.

---

## 2. Estado medido en el momento exacto en que no se ve nada

Corrida real, partida iniciada, tercera persona
(`Saved/Logs/ShotRun6.log`, captura en `Docs/evidencia/QA_TerceraPersona_SinPersonaje.png`):

```
vista=tercera  brazo_pedido=320  brazo_real=325
cuerpo malla=SK_Astraeon_Player  oculto=0  ownerNoSee=0  visible=1
       origen=(220,-0,146)  extension=(59,90,92)
cuerpo en (220,0,54)  camara en (-90,-100,202)  distancia=357  registrado=1
materiales=2
  slot 0 = /Game/Astraeon/Characters/Player/Optimized/M_Player_Character
  slot 1 = /Game/Astraeon/Characters/Player/Optimized/M_Player_Suit
animacion=AN_Astraeon_Player_All_Armature_AN_Player_Idle
```

Todo dice que debería verse. La captura muestra el interior de Ítaca con el HUD de partida
en curso y **nada en el centro del encuadre**.

Detalle relevante: `extension` cambia entre corridas —(19,92,92) en pose de referencia,
(59,90,92) y (74,82,92) con animación—, o sea que **la pose se está evaluando**. No es una
malla congelada ni degenerada.

---

## 3. Descartado con evidencia

| Hipótesis | Cómo se descartó |
|---|---|
| La malla no está asignada | log: `malla=SK_Astraeon_Player`; y `player_body_wiring.json` |
| El actor o el componente están ocultos | log: `oculto=0`, `visible=1`, `registrado=1` |
| `bOwnerNoSee` deja el cuerpo invisible para su jugador | log: `ownerNoSee=0` en tercera persona |
| El brazo de cámara colapsa y mete la cámara dentro de la malla | log: `brazo_pedido=320 brazo_real=325`; y en otra corrida 239 con la cámara a 336 cm |
| La cámara mira a otro lado | cuerpo a 357 cm de la cámara, centrado por el brazo |
| Materiales nulos | ambos slots asignados; `body_render_check.json` los da opacos y no nulos |
| Materiales transparentes | `blend_mode = BLEND_OPAQUE`, `two_sided = false` |
| El paquete no incluye el asset | el ejecutable empaquetado reporta la misma malla y materiales |
| La malla está sin animar y colapsada | `animacion=AN_Player_Idle`, y los bounds cambian con la pose |
| El cuerpo se anima con clips de otro esqueleto | **era cierto y se corrigió** (ver §4); no resolvió el síntoma |
| El escenario tapa al personaje | la captura muestra el interior despejado alrededor del punto donde debería estar |

---

## 4. Un defecto real encontrado y corregido por el camino

`UAstraeonFirstPersonRigComponent` empujaba su clip de locomoción al cuerpo de sombra:

```cpp
ShadowBody->PlayAnimation(Sequence, bLoop);   // Sequence vive en SKEL_Humanoid_A
```

Mientras el cuerpo fue el blockout humano compartían esqueleto. Al pasar el cuerpo al
protagonista dejaron de compartirlo (57 huesos contra 75) y evaluar una malla con una
secuencia de otro esqueleto no produce pose válida.

Medido antes de tocar nada, en `ContentPipeline/reports/body_render_check.json`:

```
body_skeleton   : SK_Astraeon_Player_Skeleton
idle_skeleton   : SKEL_Humanoid_A
skeletons_match : false
```

Corregido: el cuerpo tiene su propio juego de clips y `BodyCounterpart()` traduce; antes de
reproducir se comprueba que el esqueleto coincida. Cubierto por prueba. **Era un bug real,
pero el personaje sigue sin verse**, así que no era la causa del síntoma.

---

## 5. Hipótesis viva, y cómo probarla

La que queda en pie es que **el asset no tiene datos de render utilizables en juego**, pese a
que todos los indicadores de CPU son correctos. Dos variantes:

1. **LOD sin datos de render.** Los LOD1–3 se importaron con
   `SkeletalMeshEditorSubsystem.import_lod`. Si las pantallas de LOD o el `MinLOD` quedaron
   mal, a 357 cm el motor puede estar eligiendo un nivel sin geometría válida.
   *Prueba:* `BodyMesh->SetForcedLOD(1)` (fuerza LOD0) y capturar. Si aparece, es esto.

2. **Los materiales generados no compilan a nada visible.** `M_Player_Character` y
   `M_Player_Suit` se crearon por script con `MaterialFactoryNew` y
   `MaterialEditingLibrary`. Un material sin shader válido normalmente cae al material por
   defecto, pero conviene descartarlo.
   *Prueba:* ya está el parámetro `-AstraeonBasicBodyMaterial`, que fuerza
   `WorldGridMaterial`. **No llegó a ejecutarse con partida iniciada**: en el intento el
   smoke cerró el juego antes de que saltara el temporizador. Es el siguiente paso obvio.

Orden recomendado: primero la prueba 2 (ya está el parámetro, cuesta una corrida), después
la 1.

Si ninguna de las dos, el siguiente corte es aislar el asset: spawnear
`SK_Astraeon_Player` como `SkeletalMeshActor` en un mapa vacío con una luz, y capturar. Eso
separa "el asset no se dibuja nunca" de "no se dibuja en este personaje".

---

## 6. Nota de método

Perdí varias iteraciones deduciendo a partir de reportes de estado en vez de mirar un
fotograma. Los reportes decían "visible" en todos los intentos y el personaje no estaba.
La captura dentro del juego fue lo único que permitió descartar de verdad, y debió ser el
primer paso, no el sexto.

Trampas encontradas al montar el diagnóstico, por si se repiten:

- `-ExecCmds` corre antes de que exista el pawn: un comando de consola que dependa del
  personaje no se ejecuta.
- Sin iniciar partida, la vista es la del menú y la captura no dice nada del personaje.
- Iniciar partida **recrea el pawn**: el estado de cámara del pawn anterior se pierde, y con
  él cualquier temporizador que se le hubiera puesto.
- Con `-AstraeonAutoSmokeCriticalPath` el juego se cierra antes de los pocos segundos que
  necesita una captura diferida.
