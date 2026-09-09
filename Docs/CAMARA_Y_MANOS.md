# Vista en tercera persona y manos de primera persona

Fecha: 2026-09-08.

## Cámara en tercera persona — HECHA

Tecla **V**. `AstraeonPlayerCharacter` monta un `USpringArmComponent` de 320 cm sobre la
cápsula, con `SocketOffset` (0, 55, 20) para que el cuerpo no tape el centro de la pantalla
y `bDoCollisionTest` activo para que la cámara no atraviese paredes ni terreno.

Al alternar se mueven tres cosas a la vez:

| | Primera persona | Tercera persona |
|---|---|---|
| cámara activa | `FirstPersonCamera` | `ThirdPersonCamera` |
| `bOwnerNoSee` del cuerpo | true (no te ves dentro de tu malla) | false (se ve el protagonista) |
| rig de manos | visible | oculto (está pegado a la cámara de primera) |

Trampa encontrada: `SetActive(false)` en el constructor no alcanza, porque los componentes
se **auto-activan al registrarse** y la partida arrancaba en tercera persona. Hace falta
`bAutoActivate = false`.

Cubierto por `Astraeon.Art.Character.FirstPersonRigIsWired`, que alterna y comprueba las tres
cosas en ambos sentidos. El estado inicial se verifica por la bandera del personaje y no por
`IsActive`, porque en el mundo transitorio de la prueba la activación automática de
componentes no es determinista.

## Cuerpo invisible en tercera persona — CORREGIDO

Con la cámara ya funcionando, el personaje seguía sin verse: "un cuerpo vacío".

La causa la introduje yo al cambiar la malla del cuerpo. `UAstraeonFirstPersonRigComponent`
empujaba **su** clip de locomoción también al cuerpo de sombra:

```cpp
ShadowBody->PlayAnimation(Sequence, bLoop);   // Sequence vive en SKEL_Humanoid_A
```

Mientras el cuerpo era el blockout humano, compartían esqueleto y funcionaba. Al pasar el
cuerpo al protagonista dejaron de compartirlo —57 huesos contra 75— y evaluar una malla con
una secuencia de otro esqueleto no produce pose válida: la malla deja de dibujarse.

Verificado antes de tocar nada (`ContentPipeline/reports/body_render_check.json`):

```
body_skeleton   : SK_Astraeon_Player_Skeleton
idle_skeleton   : SKEL_Humanoid_A
skeletons_match : false
```

Los materiales estaban bien: `M_Player_Character` y `M_Player_Suit`, ambos opacos y
asignados. No era un problema de material.

**Fix:** el cuerpo tiene ahora su propio juego de clips, cargados de
`/Game/Astraeon/Characters/Player/Optimized/`, y `BodyCounterpart()` traduce el clip de
manos al equivalente del cuerpo. Antes de reproducir se comprueba que el esqueleto de la
secuencia coincida con el de la malla; si no, el cuerpo conserva su pose en vez de quedar
sin ninguna. `SetShadowBodyMesh()` arranca la animación al engancharse, porque el cuerpo
puede asignarse después de que el rig ya eligió su clip.

Cubierto por la prueba: el cuerpo recibe una animación propia, y esa animación **no**
pertenece al esqueleto de las manos.

## Manos de primera persona — DEFECTO ABIERTO

El jugador no ve sus manos. No es que falten: están fuera del encuadre.

Medido sobre `SK_Human_HandsFP_Blockout`
(`ContentPipeline/reports/first_person_hands.json`):

```
bounds de la malla:  centro Z = 104,6 cm,  extensión = ±59,1 (X)  ±6,0 (Y)  ±21,4 (Z) cm
```

Es decir, la malla ocupa de 83 a 126 cm de altura y **±59 cm de ancho**: son brazos colgando
a los costados de un cuerpo de pie, no manos sostenidas frente a la cara.

La cámara está a 160 cm sobre los pies (cápsula 96 + offset 64) y el rig se baja 160 cm para
apoyar su raíz en el suelo. Resultado: las manos quedan entre **35 y 77 cm por debajo** de la
cámara y hasta **59 cm a cada lado**. Mirando al frente no entran en el campo de visión.

Ningún ajuste de posición del rig lo arregla: a 30 cm de la cámara, ±59 cm de separación son
unos 63° fuera de eje por lado, más que el medio ángulo horizontal del campo de visión. Hay
que **re-autorizar las poses de brazo** del lote humano para que las manos se sostengan
delante, no reencuadrar la malla.

Es el mismo problema que se midió en el rig del protagonista: los clips levantan el brazo por
el costado en vez de llevarlo al frente. Ver `PENDIENTE_PROTAGONISTA.md`, P9.

**Mientras tanto**, la tecla V da la vía para ver al personaje completo.

## Personaje invisible: la escala perdida en la importación — CORREGIDO

Corregido lo anterior, el personaje **seguía sin verse**. Ninguna bandera de render lo
explicaba, porque el problema no era de render: la importación FBX de animaciones sueltas
perdía la conversión metros→centímetros del Armature. El esqueleto lleva `root` a escala 100
en su pose de referencia; las claves de animación quedaban a 1. En pose de referencia la
malla medía bien —de ahí que todos los diagnósticos dieran correctos—, pero al evaluar
cualquier clip la pose se encogía a 1/100: **cabeza a 1,64 cm de los pies**.

`Scripts/Editor/CharacterAnimationScale.py` reescribe las claves de `root` con la escala de
la pose de referencia, sólo ante el desajuste exacto 1 contra 100, y valida que la cabeza
quede entre 65 y 220 cm en cinco muestras del clip. Los 59 clips existentes se repararon con
`RepairCharacterPresentation.py` y todos los scripts de importación lo aplican de entrada.

Segunda causa concurrente: los materiales generados por script no declaraban
`MATUSAGE_SKELETAL_MESH`. El editor compila ese uso al vuelo; el cocinado no.

Las manos de primera persona dejaron de colgar fuera del encuadre: `AN_HandsFP_*` copian la
pose de brazos del agarre de escáner ya autorizado sobre los clips de locomoción. Y el
agarre de herramienta divide su offset en cm por la escala 100 del socket antes de aplicarlo.

Evidencia: `Docs/evidencia/QA_Personaje_TerceraPersona_Integrado.png` y
`QA_Personaje_PrimeraPersona_Integrado.png`; smoke con la tecla V real, en editor y sobre el
ejecutable, con `HeadHeightCm=163.9`. Investigación completa en
`INVESTIGACION_PERSONAJE_INVISIBLE.md`.
