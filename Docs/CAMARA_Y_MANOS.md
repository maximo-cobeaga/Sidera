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
