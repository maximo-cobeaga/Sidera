# ASTRAEON — Brief de diseño y arte

> **Actualización de producción 2026-09-06:** primer lote humano Q1 creado en Blender:
> cuerpo, manos FP, esqueleto de 57 huesos y siete animaciones simples. Importación
> Unreal transitoria aprobada, todavía sin conexión al personaje jugable. La auditoría
> de ausencia de skeletons/animación que sigue es histórica para las fuentes de arte
> y continúa aplicando al runtime. Archivos/pruebas: `Docs/HUMANOID_ART.md`.

Documento único de referencia para todo lo que hay que **modelar, animar o diseñar**.
Levantado desde el código, no desde intenciones: cada entrada dice dónde enchufa y qué
medidas reales usa el juego hoy.

> **Norte del proyecto (decisión del propietario, 2026-09-06):**
> *"Quiero un juego jugable, no un modo historia."* La historia da sentido y dirección, pero
> **al terminarla se sigue jugando**. Todo diseño debe sostener el bucle a largo plazo —
> supervivencia, caza, construcción, exploración— y no sólo una primera pasada narrativa.

Documentos hermanos: `Docs/ASSETS_PENDIENTES_DISENO_ANIMACION.md` (inventario detallado),
`Docs/ART_ASSET_MASTER_PLAN.md` (plan de producción), `Docs/BLENDER_PIPELINE.md` (pipeline).

---

## 0. Estado real de la capa visual

| Capa | Estado |
|---|---|
| Esqueletos y animación | **No existe ninguno.** Cero `SkeletalMesh`, cero `AnimSequence`. |
| VFX | **Ninguno.** Sin Niagara ni partículas. |
| Audio | **Ninguno.** Ni una reproducción de sonido en todo el proyecto. |
| UI | Texto dibujado en C++ (`AAstraeonHUD`). Sin UMG. |
| Mallas | Kit de Ítaca + 7 blockouts regionales. **El resto son cubos y esferas del motor.** |

---

## 1. Medidas canónicas — respetarlas o se rompe el juego

Sacadas del código. Son contratos, no sugerencias.

| Elemento | Medida |
|---|---|
| Cápsula del jugador | radio 42 cm, semialtura 96 cm → **1,92 × 0,84 m** |
| Altura de ojos | **~1,60 m** |
| Escalón máximo caminable | **45 cm** (`AAstraeonTerrainField::GetMaxWalkableStepCm`) |
| Estancia de Ítaca | **8 × 6 × 2,8 m**, grilla 2 m, panel 0,12 m |
| Hueco de escotilla | **1,3 × 2,2 m** |
| Casco de la nave | **9 × 6 × 2,2 m** (placeholder actual) |
| Umbra Grazer | **1,4 × 0,8 × 0,7 m** |
| Bloque de terreno | **7 m** de lado |
| Montañas | hasta **52 m** |
| Muro construible | **3,2 × 0,4 × 2,6 m** |
| Plataforma construible | **3,2 × 3,2 × 0,3 m** |
| Pilar construible | **0,6 × 0,6 × 3,0 m** |

**Regla dura:** los `FName` de ítems, recursos y marcadores son **claves de guardado y de
recetas**. La presentación puede cambiar; el id **nunca**.

---

## 2. Personaje jugable — prioridad 1

Hoy es una cápsula invisible con una cámara. Es lo que el jugador ve el 100 % del tiempo.

**Modelar**
- **Manos en primera persona** (lo mínimo imprescindible).
- Traje modular: torso, piernas, botas, guantes, casco, mochila.
- Cuerpo completo — **ahora sí necesario**: el propietario pidió cámara en tercera persona,
  y sin cuerpo no hay nada que mostrar.

**Esqueleto `SKEL_Humanoid_A`** — no existe y **bloquea toda animación humana**. Es el
primer entregable del pipeline.

**Animar**: idle, caminar, **correr**, saltar, aterrizar, interactuar (`E`), escanear
(click izq.), disparar (click der.), **extraer con taladro**, **martillar al construir**,
**comer una ración** (`F`), cambiar objeto de la mano.

**Enchufa en**: `AAstraeonPlayerCharacter`.

---

## 3. Objetos de mano — el sistema ya existe y no tiene ni un modelo

El juego ahora exige **llevar el objeto en la mano** para usarlo (barra rápida, teclas 1-6).
Eso significa que estos objetos **se ven permanentemente en pantalla** y son la lectura
principal del estado del jugador.

| Objeto | Id (no tocar) | Uso |
|---|---|---|
| Escáner | *(implícito)* | Click izq. Nunca tuvo modelo. |
| Cortadora de pulso | `weapon_pulse_cutter` | Click der. dispara |
| Taladro de núcleo | `tool_core_drill` | `E` sobre veta profunda o suelo |
| Martillo de obra | `tool_build_hammer` | `B` entra en modo construcción |
| Maza de demolición | `tool_demolition_maul` | Click der. derriba |
| Ración | `ration_pack` | `F` come |
| Resonador de señal | `signal_resonator` | Objeto de misión |

Cada uno necesita: modelo en primera persona, versión en tercera persona (a la cadera o en
la espalda cuando no está en mano) y animación de uso.

---

## 4. Módulos para criaturas — sistema modular

El propietario pidió **módulos para crear mobs y razas**. Este es el contrato propuesto.

### 4.1 Principio

Una criatura **no es un modelo**, es una **combinación de módulos** elegida por seed. El
juego ya genera regiones deterministas; la ecología debe seguir la misma lógica: la misma
seed produce la misma fauna.

### 4.2 Familias de esqueleto

Cada familia es un esqueleto propio; **no se comparten entre anatomías distintas**.

| Familia | Esqueleto | Uso |
|---|---|---|
| Cuadrúpedo | `SKEL_Quadruped_A` | Umbra Grazer y herbívoros/depredadores terrestres |
| Bípedo no humano | `SKEL_Biped_B` | Fauna erguida, futuras razas alienígenas |
| Humanoide | `SKEL_Humanoid_A` | Jugador, NPC y razas humanoides |
| Reptante | `SKEL_Crawler_C` | Fauna baja, sin extremidades erguidas |
| Volador | `SKEL_Flyer_D` | Fauna aérea |

### 4.3 Ranuras de módulo (por familia)

Nomenclatura: `SM_Crt_<Familia>_<Ranura>_<Variante>`
Ejemplo: `SM_Crt_Quad_Head_Grazer01`

| Ranura | Obligatoria | Qué define |
|---|---|---|
| `Torso` | sí | Volumen y proporción base; ancla del resto |
| `Head` | sí | Lectura de especie: es lo que el jugador mira |
| `Limb_Front` / `Limb_Rear` | sí | Locomoción y silueta |
| `Tail` | no | Equilibrio visual |
| `Armor` | no | Placas; sube resistencia percibida |
| `Sensor` | no | Ojos, antenas, órganos luminosos |
| `Appendage` | no | Cuernos, espinas, aletas |

**Sockets de anclaje** (nombres fijos, el código los usará):
`socket_head`, `socket_limb_fl`, `socket_limb_fr`, `socket_limb_rl`, `socket_limb_rr`,
`socket_tail`, `socket_armor_01..03`, `socket_sensor_01..02`.

### 4.4 Reglas para que el modular no se rompa

- Todos los módulos de una ranura comparten **el mismo pivote y la misma orientación**.
- Rango de escala permitido por módulo: **0,9–1,1**. Fuera de eso los pies dejan de tocar el
  suelo y el rig se rompe.
- Cada módulo cierra su propia silueta: **nada de agujeros** que dependan del vecino.
- Paleta por variante mediante instancia de material, **no por textura nueva**.
- Presupuesto LOD0: **≤15k tris** por criatura completa, 2 slots de material, atlas 2K.

### 4.5 Animaciones mínimas por familia

`idle`, `caminar`, `correr`, `alerta`, `amenaza`, `recibir daño`, `morir`, `retirada`.

Estados ya implementados en código que la animación debe cubrir:
`Patrolling`, `Alert`, `Threatening`, `Disengaging` — hoy se comunican **cambiando el tamaño
de una esfera**, que es el placeholder más evidente que queda.

**Enchufa en**: `AAstraeonCreatureActor` + `FAstraeonCreatureProfile` (que ya tiene
`MaxHealth`, `HarvestItemId`, `HarvestQuantity`, radios de alerta/amenaza y patrulla).

---

## 5. Razas y civilizaciones — contrato modular

Mismo principio que las criaturas, sobre `SKEL_Humanoid_A`.

| Ranura | Qué define |
|---|---|
| `Head` | Identidad de la raza; es lo que se recuerda |
| `Body` | Complexión (delgada, robusta, alargada) |
| `Skin` | Paleta y material; variación individual |
| `Garment` | Cultura: ropa, insignias, herramientas colgadas |
| `Accessory` | Rango, oficio, edad |

**Regla de compatibilidad**: una raza sólo comparte esqueleto con el jugador si sus
proporciones lo permiten (mismo largo relativo de extremidades ±10 %). Anatomías fuera de ese
rango **exigen familia propia**, no un reescalado.

**Nomenclatura**: `SM_Race_<Raza>_<Ranura>_<Variante>`.

**Aviso de alcance**: no hay código de NPC ni de civilizaciones todavía. Esto es contrato
para cuando exista; conviene tener 1–2 razas jugables antes de diseñar la sociedad entera.

---

## 6. Ítaca — nave y estaciones

La nave **vuela y aterriza de verdad**, y su casco es un cubo gris.

**Modelar**: casco exterior coherente con el interior de 8 × 6 × 2,8 m, motores con anclaje
para VFX, tren de aterrizaje, **hoja de escotilla articulada** (hoy el hueco está siempre
abierto), puesto de pilotaje visible desde dentro.

**Animar**: despegue, aterrizaje, tren desplegando, escotilla abriendo/cerrando, ralentí de
motores.

**Estaciones interiores** (todas se mueven con la nave):

| Estación | Estado | Id |
|---|---|---|
| Consola ARGOS | blockout Q1 hecho | `itaca_argos_console` |
| Consola de pilotaje | **cubo** | `itaca_pilot_console` |
| Mesa de fabricación | **cubo** | `itaca_fabricator` |
| Escotilla | marco Q1, **sin hoja** | `itaca_surface_hatch` |

---

## 7. Construcción — piezas del jugador

Sistema funcionando; las piezas son cubos marrones. Necesitan leer como **obra humana de
regolito compactado**, distinta del terreno y del metal de la nave.

- `Muro` 3,2 × 0,4 × 2,6 m — 4 ladrillos
- `Plataforma` 3,2 × 3,2 × 0,3 m — 3 ladrillos
- `Pilar` 0,6 × 0,6 × 3,0 m — 2 ladrillos

Deben **encastrar entre sí** en la grilla y rotar de a 45° sin dejar huecos.
Falta además una **versión fantasma** (translúcida) para la vista previa de colocación.

**Pedido pendiente del propietario**: *"suavizar las capas de terreno; los cuadrados
elevados generan un borde negro visible desde arriba"*. Es la cara vertical de cada bloque en
sombra. Se resuelve del lado de arte con material/normales, o del lado de código pasando de
bloques instanciados a una malla continua — **decisión aún abierta**.

---

## 8. Entorno

- Suelo real (hoy: placa gris de 1200 × 1200 m).
- **Terreno**: bloques de 7 m instanciados. Necesita material que disimule la cara vertical.
- **Montañas**: hasta 52 m, no escalables, son barreras y referencia visual.
- Rocas y landmarks (hoy: cubos ocres).
- **Cielo**: no hay skybox ni atmósfera física, sólo luz direccional + bruma.

---

## 9. Interfaz — reemplazo completo por UMG

Todo es texto en C++. Vistas ya funcionando que necesitan diseño real:

HUD de estado (ambiente, amenazas, protección, **saciedad**), **barra rápida de objetos en
mano**, inventario por categorías, bitácora, mesa de fabricación, modo construcción, HUD de
vuelo, menú inicial, mira central.

---

## 10. VFX y audio — capas inexistentes

**VFX**: pulso y impacto del arma, haz del escáner, chispas del taladro, polvo al construir,
empuje de motores, polvo de aterrizaje, partículas ambientales, daño al traje.

**Audio**: **no hay ni un solo sonido**. Pasos, viento, motores, disparo, impacto, criatura,
interfaz, alarma de oxígeno y de hambre.

---

## 11. Orden sugerido

1. **`SKEL_Humanoid_A` + manos en primera persona.** Desbloquea todo lo demás.
2. **Objetos de mano** (escáner, cortadora, taladro, martillo, maza, ración) con animación de
   uso: ahora se ven siempre en pantalla.
3. **Cuerpo completo del jugador** — habilita la cámara en tercera persona pedida.
4. **Criatura**: `SKEL_Quadruped_A` + módulos + las ocho animaciones de estado.
5. **Ítaca**: casco, hoja de escotilla, despegue/aterrizaje.
6. Estaciones interiores, piezas de construcción, terreno y cielo.
7. UMG, VFX y audio.
