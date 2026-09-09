# Criatura — modelo, rig y estados

2026-09-06. `Umbra Grazer` deja de ser una esfera escalada. Calidad **Q1**, original y
reproducible, no arte final. Cierra la parte de presentación de AC-06.

## Entrega

- **`SKEL_Quadruped_A`**: 37 huesos. Raíz al suelo, pelvis, tres de columna, dos de cuello,
  cabeza y mandíbula, cola de cuatro segmentos, y cuatro cadenas de tres articulaciones más
  dedo. Sockets `socket_plate_back`, `socket_horn_l`, `socket_horn_r`, `socket_sensor`, y
  las interfaces modulares del plan maestro (cuello, pelvis, hombros, raíz de cola).
- **`SK_Creature_UmbraGrazer_Blockout`**: 5.080 triángulos, 2 materiales, 1,41 × 0,51 × 0,71 m.
- **`SK_Creature_UmbraGrazer_Plated_Blockout`**: la variante que pide AC-06 — 5.404 triángulos,
  mismo esqueleto, paleta más fría y tres placas dorsales ligeras.
- Siete acciones a 30 FPS, una por estado real del código:

  | Clip | Duración | Estado en `EAstraeonCreatureAwarenessState` |
  | --- | --- | --- |
  | `AN_Creature_Graze_Blockout` | 3,0 s | `Patrolling`, detenida |
  | `AN_Creature_Walk_Blockout` | 1,333 s | `Patrolling`, en movimiento |
  | `AN_Creature_Alert_Blockout` | 2,0 s | `Alert` |
  | `AN_Creature_Threaten_Blockout` | 1,0 s | `Threatening` |
  | `AN_Creature_Flee_Blockout` | 0,667 s | `Disengaging` |
  | `AN_Creature_Hit_Blockout` | 0,6 s | impacto no letal |
  | `AN_Creature_Death_Blockout` | 1,2 s | muerte |

Silueta baja y ancha, barril pesado, cuello corto y cabeza por debajo de los hombros: un
herbívoro, no un depredador. Los ojos son el material de piel sobre la carcasa clara de la
cabeza, así que leen oscuros sin gastar la tercera ranura de material que el presupuesto no
permite.

## El contrato de apoyo, y por qué costó la muerte

Cada fotograma se posa, se mide y se apoya: el generador baja o sube la pelvis hasta que el
punto más bajo de la malla toca el suelo, y sólo después suma el vaivén que el clip declara
explícitamente como `lift`. Ningún clip puede flotar ni hundirse por accidente, y todo
movimiento vertical tiene que estar declarado.

Eso hace que **el cuerpo sólo baje si la pose lo baja de verdad**, lo que convirtió la muerte
en el clip difícil. Tres intentos fallaron y cada uno enseñó algo que quedó en el código:

1. Girar el torso de costado no bajó nada. La grupa está centrada exactamente en el pivote
   de la pelvis, así que rotar ahí la deja donde estaba.
2. Plegar las patas hacia atrás tampoco: quedan apoyadas y sostienen el cuerpo.
3. Abrir muslo y caña en el mismo sentido mandó la pata por encima del lomo, porque la caña
   hereda el giro del muslo.

Lo que funciona es que las patas se abran hasta quedar horizontales a la altura del propio
torso, y que el cuello baje sólo hasta apoyar la cabeza al nivel del vientre: más que eso y
el animal queda colgado de su propia cabeza. El requisito real resultó ser que el **alto
total del cadáver** entre en 0,50 m, no que una pieza concreta descienda. El validador lo
comprueba y, si falla, ahora reporta el punto más alto entero —posición incluida— porque
saber si lo que sobresale es el lomo, una pata o la cabeza es la diferencia entre corregir
la pose y adivinar otro ángulo.

## Integración en el juego

`AAstraeonCreatureActor` cambió de esfera visible a envolvente invisible más malla animada:

- La raíz sigue siendo el `UStaticMeshComponent` de siempre con `BlockAll` y la escala
  1,4 × 0,8 × 0,7 m, ahora oculto. **Disparo, escaneo y daño por contacto conservan
  exactamente el mismo alcance**; el arte cambia la presentación, no lo que se puede tocar.
- `CreatureArt` es el `USkeletalMeshComponent`, sin colisión, con el pivote en las patas.
- Como el pivote pasó a las patas, `HoverHeightCm` (60 cm) y el seno que falseaba el vaivén
  desaparecieron: la cota del terreno es directamente la del actor y el balanceo lo trae la
  animación. Las distancias de percepción y contacto se miden desde `GetBodyCenterCm()`,
  que es donde estaba el actor antes, para no mover ningún umbral.
- La variante se elige por hash del nido (`SetSpawnPointId`), no al azar: la misma seed
  devuelve el mismo animal en el mismo sitio partida tras partida.
- **Patrullar ahora alterna**: cuatro segundos quieta pastando de cada diez. Sin eso el clip
  de pastar no se vería nunca y la región se leía como un carrusel que no para.
- **Matar deja cadáver**: `FireWeapon` llama a `BeginDeathSequence()` en vez de `Destroy()`.
  La criatura apaga su colisión, reproduce la caída y recién entonces se retira. Antes se
  destruía en el mismo frame del disparo, así que matar era sólo una línea de texto.

La escala como señal de estado (`ComputeStateVisualScale`) se eliminó: era el placeholder
que las animaciones vienen a reemplazar.

## Reproducción

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/Blender/generators/creature_blockout.py
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -run=pythonscript -script='C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\PrepareCreatureBlockoutAssets.py' -unattended -nullrhi -nosplash -nop4 -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\CreaturePackages.log'
& 'C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe' Tools/Blender/build_creature_manifest.py
```

## Verificación

- Blender: 6 controles negativos rechazados (escala sin aplicar, origen desplazado, pesos sin
  normalizar, vértice sin peso, socket ausente, cola recortada), determinismo de geometría
  comprobado por doble generación, presupuesto de triángulos y silueta, y los 300 fotogramas
  de los siete clips con deformación finita, apoyo en el suelo y cierre exacto de los cinco
  ciclos. 9 roundtrips FBX con jerarquía, escala, skin y duración conservadas.
- Unreal 5.7.4: 14 paquetes guardados, esqueleto de 37 huesos, ambas mallas de 141,1 cm sobre
  el mismo esqueleto, siete `AnimSequence` con duración `(frames-1)/fps`. Exit 0, commandlet
  0 errores / 0 warnings.
- `AstraeonEditor Win64 Development`: compilación limpia.
- Automation `Astraeon.*`: **50 encontrados, 50 exitosos**, exit 0.
- Smoke crítico editor-game:
  `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- Test nuevo `Astraeon.Art.Creature.PresentationPreservesGameplay`: comprueba los 37 huesos y
  la raíz, que sobrevivan las cuatro cadenas, la cola, la mandíbula y los sockets, que el arte
  no tenga colisión y el envolvente sí conserve la suya y su silueta, que el centro de cuerpo
  esté 35 cm sobre las patas, que los siete clips carguen y apunten al mismo esqueleto, que la
  variante sea estable por nido, y que un cadáver deje de bloquear sin destruirse en el frame
  del disparo.

Evidencia: `ContentPipeline/reports/creature_blockout_validation.json`,
`creature_content_packages.json` y `ContentPipeline/creature_asset_manifest.json`.
Previews en `ContentPipeline/Generated/CreatureBlockout/`: una por clip más `Preview_Base.png`
y `Preview_Variant.png`.

## Pendiente de revisión humana y límites

- **Revisar las previews.** Hay una por clip. La silueta de reposo y la caída se ven bien; el
  gesto de amenaza (cabeza baja, mandíbula abierta, peso atrás) **se distingue del reposo pero
  es sutil**, y puede necesitar más contraste para funcionar como aviso a distancia.
- El recule de `Hit` levanta la cabeza hasta 1,07 m en un animal de 0,71 m: es legible, pero
  está en el límite de lo plausible.
- Sin IK de pies ni mezcla por velocidad: los clips cortan, igual que en el lote humano. La
  marcha es un trote diagonal y el galope de huida conserva un apoyo, sin fase de vuelo
  refinada.
- Sin PhysicsAsset, sin ragdoll, sin colisión ajustada a la silueta: el cadáver es un clip,
  no física. Sin VFX ni audio de impacto, muerte o pisadas.
- Anatomía y materiales son Q1: colores planos, UV de prototipo, sin texturas. Las placas de
  la variante son cajas biseladas, no un catálogo de equipo intercambiable.
- El contrato determinista de módulos por `EntitySeed` del plan maestro sigue sin implementarse:
  hoy la variación es una elección entre dos mallas, no un generador paramétrico.
