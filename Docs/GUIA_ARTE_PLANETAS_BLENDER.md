# Guía de arte — Planetas procedurales (Blender → Unreal)

> **Sincronizado con el [ADR 0004](ADR/0004-planetas-esfericos-fundacionales.md) el 2026-09-09.**
> Su **supuesto técnico anticipó la dirección actual**: cube-sphere procedural por caras
> normalizadas, blending por datos de vértice generados en C++ y no por splat maps de Landscape,
> y Blender que no modela el planeta final. Todo eso pasó de supuesto a requisito fundacional y
> se conserva **sin cambios**.
>
> Cambiaron dos cosas: los tamaños de referencia se reclasificaron como laboratorio (ver §Tamaños)
> y la prioridad — este trabajo ya no espera detrás del MVP plano.

**Alcance de este documento:** especificación de assets y pipeline de Blender para el sistema de planetas (tamaño variable, terreno destructible/regenerable, biomas combinables por seed). Es preparación de arte: define qué produce Blender, no cuándo se construye el runtime planetario. Los seis kits de bioma son entregable de la **Fase 4**; las mallas de referencia y los props de escala se usan antes, para armar los test levels que validan el sistema a medida que se programa (`PHASE_STATUS.md`).

**Supuesto técnico asumido** (confirmado con el usuario; **elevado a requisito por el ADR 0004**): terreno = malla *cube-sphere* procedural (`ProceduralMeshComponent`, 6 caras deformadas por altura y normalizadas a esfera), destrucción/regeneración vía Chaos + remesh local, sin plugin de vóxeles de terceros. El blending entre biomas se resuelve en shader por **vertex color / datos por vértice generados en C++**, no por splat maps de Landscape pintados a mano por planeta. Blender no modela el planeta final: produce la malla de referencia, los sets de materiales tileables por bioma y los props.

> Matiz posterior al ADR 0004: `ProceduralMeshComponent` es **experimental** en 5.7.4. Se usa para el prototipo detrás de la abstracción `IPlanetPatchMeshBackend`, y la elección de producción se decide por ADR propio después de medir. Eso no afecta a lo que produce Blender.

**Biomas propuestos** (ajustables, 6 para cubrir variedad visual y de gameplay sin dispersar el trabajo de arte):

| Código interno (usar en nombres de asset) | Nombre en juego (ES) | Identidad visual |
|---|---|---|
| `Desert` | Desértico | Dunas, arena, roca erosionada |
| `Rocky` | Rocoso | Roca expuesta, cañones, cárcavas |
| `Forest` | Boscoso | Vegetación densa, musgo, tierra orgánica |
| `Ice` | Helado | Nieve, hielo, cristales de escarcha |
| `Volcanic` | Volcánico | Basalto, obsidiana, ceniza, lava solidificada |
| `Saline` | Salino | Llanuras de sal, formaciones cristalinas |

Usar el código interno (inglés) en nombres de archivo/asset — mantiene consistencia con el código C++ del proyecto (enums/identificadores en inglés). El texto visible en UI sigue en español.

---

## 1. Especificaciones técnicas de assets

### 1.1 Malla base del planeta (solo referencia, no gameplay final)

El planeta real lo genera C++ en runtime. Blender entrega una **malla de referencia por tamaño** para probar escala, curvatura, materiales y props antes de que el generador exista, y para usar en los test levels iniciales.

- Topología: cubo subdividido (cube-sphere), 6 caras, grid de quads uniforme por cara, vértices normalizados a esfera (sin escalado no uniforme por cara).
- Resolución de grid de referencia: 33×33 vértices por cara (32 segmentos) alcanza para validar curvatura y materiales sin pesar el viewport. No hace falta más detalle: el mesh real de gameplay lo genera el motor.
- Todo quads, sin ngons, sin normal smoothing manual raro (smooth shading estándar + normales recalculadas hacia afuera del centro).
- UVs: no se unwrapean para textura de superficie (el shader va a usar **triplanar**, ver 1.2). Dejar un UV0 básico por cara (proyección plana) solo para uso de debug/lightmap, no para color.
- Un archivo FBX por tamaño: `SM_Planet_Ref_Small`, `SM_Planet_Ref_Medium`, `SM_Planet_Ref_Large`, `SM_Planet_Ref_Giant`.

> **Reclasificado por el [ADR 0004](ADR/0004-planetas-esfericos-fundacionales.md) el 2026-09-09.**
> Los cuatro tiers de abajo **dejaron de ser tamaños objetivo** y pasaron a ser **escalas de
> laboratorio**: sirven para mallas de referencia y para leer curvatura, no para dimensionar un
> planeta de producción. Un planeta de producción no es una malla de Blender: es un cube-sphere
> por patches generado en runtime, y su radio es un **dato**, nunca el `Scale` de una malla.
>
> Los tiers de ingeniería vigentes están en `PLAN_TRANSICION_EJECUCION.md` §4, Fase 1:
>
> | Tier | Radio | Propósito |
> |---|---:|---|
> | Lab | 10 km | Depuración rápida |
> | Target | 500 km | Prueba principal de escala |
> | Stress | 2500 km | Precisión y cambio de representación |

Tamaños de referencia **de laboratorio** (radio, diseñado para que la curvatura se note caminando — **no** son escalas astronómicas reales ni tamaños de planeta objetivo):

| Tier de laboratorio | Radio | Diámetro | Uso |
|---|---|---|---|
| Small (Pequeño) | 150 m | 300 m | Malla de referencia, lectura de curvatura cerrada |
| Medium (Mediano) | 400 m | 800 m | Malla de referencia por defecto |
| Large (Grande) | 900 m | 1800 m | Curvatura sutil, más superficie por bioma |
| Giant (Gigante) | 2000 m | 4000 m | Curvatura casi imperceptible a pie |

Construir la malla de referencia directamente a esta escala en metros — no escalar "a ojo" después.

### 1.2 Materiales y texturas por bioma

Cada bioma es un **set de material tileable triplanar**, no una textura única por planeta.

- Formato de textura fuente: PNG o TGA de 16 bits para mapas con precisión (height), 8 bits para el resto.
- Resolución: 2048×2048 para Albedo/BaseColor y Normal; 2048×2048 para el ORM empaquetado (R=AO, G=Roughness, B=Metallic); 1024×1024 para Height (si se usa parallax/blend por altura).
- Densidad de texel objetivo: la textura de 2K debe cubrir un tile de **4×4 m** en el mundo (≈512 px/m). Mantener esta densidad igual en todos los biomas para que no se note el cambio de escala al cruzar una transición.
- Cada bioma necesita mínimo **2 capas de suelo** para que el shader pueda romper la repetición dentro del propio bioma (ej. "suelo base" + "roca expuesta/parche"), mezcladas por un mapa de detalle (ver abajo). Esto es aparte del blending *entre* biomas.
- Espacio de color: Albedo y emissive en sRGB; Normal, ORM y Height en **no-color/lineal** (marcar así al importar a Unreal, es un error común que arruina el sombreado).

**Mapas de máscara/detalle (reemplazan al splat map clásico):**

Como el blending entre biomas lo calcula C++ por vértice, Blender no pinta un splat map por planeta. Lo que sí hay que producir por bioma es un **mapa de detalle tileable** (no por planeta, reutilizable en cualquier instancia del bioma) que el shader usa para romper la mezcla entre las 2 capas de suelo del punto anterior y para variar la transición entre biomas en el borde:

- `T_<Bioma>_DetailMask`: 1024×1024, tileable, empaquetado RGB — R = variación macro (parches grandes tipo roca/arena), G = detalle fino (motas, salpicado), B = ruido de borde (para que la línea de transición entre biomas no sea recta). No-color, sin sRGB.
- Generar con textura procedural en Blender (nodos, no foto) para garantizar tiling perfecto sin costuras.

### 1.3 Props por bioma

Los props se generan/instancian en runtime sobre la superficie curva; en Blender solo se modela el catálogo, no la distribución.

Reglas comunes a todos los props:

- Pivote en la base del objeto, centrado en X/Y, Z=0 en el punto de apoyo (el motor alinea el pivote a la normal de la esfera + variante de rotación aleatoria en Z). Ningún prop puede depender de "quedar bien" solo desde un ángulo — van a rotar 360° sobre su eje al instanciarse.
- Escala no uniforme baneada en el mesh final: si necesitás variedad de tamaño, modelá 2-3 variantes de malla en vez de escalar de forma no uniforme una sola (deforma normal maps y rompe el fracturado de Chaos).
- Presupuesto de triángulos LOD0: rocas pequeñas 150–400, rocas medianas 500–1200, rocas grandes/formaciones 1500–4000, vegetación individual (tronco+copa) 400–1000, vegetación tipo "card" (pasto/arbustos con alpha) 20–80 por card.
- Solo se modela LOD0 en Blender; LOD1/LOD2 se generan con el auto-LOD de Unreal salvo que el auto-LOD rompa la silueta (revisar caso a caso en rocas grandes).
- Colisión: no autorizar colisión custom en Blender para props normales (usar colisión convexa automática de Unreal al importar). Excepción: los props marcados como destructibles (ver 1.4) necesitan malla sólida cerrada, sin eso Chaos no puede fracturarlos.

Catálogo mínimo por bioma (6 biomas × esta tabla = set inicial completo):

| Categoría | Cantidad mínima | Notas |
|---|---|---|
| Roca pequeña | 3 variantes | Dispersión de detalle, densidad alta |
| Roca mediana | 2 variantes | Densidad media, candidatas a destructibles |
| Roca grande / formación | 1–2 variantes | Landmark visual, colisión simple obligatoria |
| Vegetación o detalle de suelo primario | 2–3 variantes | Vegetación si el bioma la tiene (Forest, Desert); si no, cristales/depósitos (Ice, Volcanic, Saline) |
| Vegetación o detalle secundario | 1–2 variantes | Cards de detalle fino (pasto, líquenes, ceniza acumulada) |
| Prop único de bioma | 1 | Elemento que solo aparece en ese bioma y lo hace reconocible en foto (geiser volcánico, cristal salino gigante, árbol helado, etc.) |

### 1.4 Props destructibles

- La destrucción se resuelve con **fractura Chaos en runtime**, no con chunks pre-fracturados a mano — así el pipeline de arte no se duplica por prop.
- Requisito de malla: sólida, manifold, cerrada (watertight), sin geometría interior duplicada, normales consistentes hacia afuera. Chaos falla o genera basura sobre mallas abiertas.
- Poli-budget algo más bajo que el equivalente no-destructible de su categoría (Chaos genera geometría interna extra al fracturar): rocas medianas destructibles ≤ 800 tris, grandes ≤ 2500 tris.
- Marcar el asset con sufijo `_Destructible` en el nombre para que quede claro en Content Browser cuál necesita el setup de Chaos.
- No hace falta modelar el estado "post-destrucción" ni el estado "regenerado" — ambos son el mismo mesh (Chaos fractura y el terreno vuelve al mismo mesh original al regenerar).

---

## 2. Test levels en Unreal (orden de armado)

Armar en este orden: cada nivel aísla una variable nueva antes de combinarla con las anteriores. No conviene saltar al nivel 4 sin haber validado 1–3, porque si algo falla ahí no vas a poder distinguir si es problema de escala, de material o de mezcla de biomas.

| # | Nivel | Qué valida | Assets mínimos necesarios |
|---|---|---|---|
| 1 | `TL_00_ReferenceScale` | Escala y curvatura percibida a pie en cada tamaño de planeta | Los 4 `SM_Planet_Ref_*`, material checker simple, personaje/mannequin para referencia de altura |
| 2 | `TL_01_MaterialBiomeSingle` | Un material de bioma solo, triplanar, sin costuras visibles al cruzar caras del cube-sphere | 1 set de material completo (1 bioma) + `SM_Planet_Ref_Medium` |
| 3 | `TL_02_BiomeBlend` | Transición entre 2 biomas (ancho y suavidad del borde) usando vertex color pintado a mano en Blender como sustituto temporal del dato que va a generar C++ | 2 sets de material de bioma + una malla de prueba con gradiente de vertex color pintado |
| 4 | `TL_03_PropScatterSingleBiome` | Densidad, escala, colisión y LOD de props sobre superficie curva, 1 bioma | Catálogo completo de props de 1 bioma + su material de suelo |
| 5 | `TL_04_AllBiomesOnePlanet` | Combinación aleatoria de los 6 biomas en un mismo planeta (una seed) | Los 6 sets de material + los 6 catálogos de props completos |
| 6 | `TL_05_PropDestruction` | Fractura Chaos de un prop individual: performance, cantidad de fragmentos, colisión resultante | 1–2 rocas `_Destructible` representativas |
| 7 | `TL_06_TerrainDestructionRegen` | Deformación real del terreno (cavar/romper) y su regeneración a estado original | Material de 1 bioma + props de ese bioma (reusar assets de niveles 2–4) |
| 8 | `TL_07_SizeTierSweep` | Repetir el planeta completo (nivel 5) en los 4 tamaños, confirmar que curvatura, LOD y performance escalan bien | Todo lo anterior + los 4 `SM_Planet_Ref_*` |
| 9 | `TL_08_PerfStress` | Múltiples planetas o radio de vista lejano con muchas instancias de props, medir frame time | Set completo de assets ya construido, sin assets nuevos |

Nota: los niveles 1–4 pueden convivir con placeholders (cubos/material checker) para las partes que todavía no tengan arte final — no hace falta esperar a tener los 6 biomas terminados para empezar a probar el sistema en el nivel 1.

---

## 3. Restricciones técnicas Blender → Unreal (rompen el pipeline si no se respetan)

- **Unidades:** escena de Blender en métrico, Unit Scale = 1.0, 1 unidad de Blender = 1 metro. No tocar la unidad para "que se vea más grande" — la escala se ajusta modelando al tamaño real en metros.
- **Aplicar transform antes de exportar:** `Ctrl+A` → Rotation & Scale (y Location si el pivote quedó movido sin querer) sobre todo objeto antes de exportar a FBX. Un objeto con rotación/escala sin aplicar entra rotado o con doble escala a Unreal.
- **Exportación FBX:** Forward = `-Y`, Up = `Z`, escala de exportación 1.0. Al importar en Unreal, Import Uniform Scale = 1.0 (no 100) si se exportó con esta configuración estándar — si algo entra 100 veces más grande o más chico, el problema está acá, no en el asset.
- **Origen/pivote:** para props de suelo, pivote en la base (Z=0 en el punto de apoyo, XY centrado). Para el mesh de referencia del planeta, pivote en el centro exacto de la esfera. Un pivote descentrado rompe el alineado automático a la normal de superficie.
- **Geometría:** solo quads y tríangulos, cero ngons. Mallas manifold/cerradas obligatorias en todo lo marcado `_Destructible`. Nada de geometría interior oculta duplicada (multiplica el poly count sin motivo y confunde a Chaos).
- **Sin escala no uniforme horneada:** si un objeto quedó estirado en un solo eje al modelarlo, aplicar el transform igual pero revisar que el normal map no haya quedado distorsionado — mejor evitarlo modelando variantes en vez de escalar.
- **Un FBX por asset final** (no exportar la escena completa de Blender en un solo FBX): mantiene el reimport limpio y evita arrastrar objetos de trabajo (referencias, gizmos, cámaras) a Unreal.
- **Naming conventions** (prefijo + bioma en inglés + variante):
  - Mallas: `SM_Rock_<Bioma>_<Tamaño>_<##>` (ej. `SM_Rock_Desert_Medium_02`), `SM_Rock_<Bioma>_<Tamaño>_<##>_Destructible`, `SM_Veg_<Bioma>_<Tipo>_<##>`, `SM_Planet_Ref_<Tier>`.
  - Texturas: `T_<Bioma>_Ground_A` / `_N` / `_ORM` / `_H`, `T_<Bioma>_DetailMask`.
  - Materiales: `M_<Bioma>_Ground` (material maestro), `MI_<Bioma>_Ground_<Variante>` (instancias).
  - Carpeta de destino sugerida en Content Browser: `Content/Astraeon/Planets/Biomes/<Bioma>/{Meshes,Materials,Textures}`.
- **Espacio de color al importar:** confirmar en Unreal que Normal/ORM/DetailMask quedan marcados como no-sRGB. Es el error más común que hace que un bioma se vea "lavado" o con specular raro y no es un problema del asset de Blender sino de la configuración de importación — pero conviene revisarlo cada vez porque Unreal a veces adivina mal el flag por el nombre del archivo.
