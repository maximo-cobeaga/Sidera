# Plan maestro de assets — ASTRAEON

Fecha: 2026-09-05. Alcance de esta entrega: planificación de familias y primer kit **blockout**, no arte final ni cierre del MVP.

## Auditoría y límites

Fuentes: `AGENTS.md`, `MVP_LA_PRIMERA_SENAL.md`, `DEVELOPMENT_STATE.md`, `BACKLOG.md`, `GAME_DESIGN_MASTER.md`, ADR 0001, `GUIA_ARTE_PLANETAS_BLENDER.md`, código y scripts locales. El prompt maestro solicita iniciar producción; el documento maestro se define a sí mismo como visión futura. Se conserva la prioridad del alcance MVP incluso donde la jerarquía al final del prompt difiere del contrato operativo.

Estado observado:

- `Content/Maps/L_AstraeonBootstrap.umap` es el único asset de Content encontrado. `Scripts/Editor/CreateBootstrapMap.py` lo construye con primitivas del motor; no regenerarlo sobre el existente.
- `graphics/test_astra.py` y `graphics/astra_test.blend` son la prueba previa de bpy, no una biblioteca de producción. Se conservan.
- No existía `Tools/Blender`, manifiesto ni `Docs/MODULAR_ASSET_GENERATION.md`. Este último se añade como contrato inicial, sin atribuirle una aprobación anterior.
- Personaje C++: cápsula radio 42 cm, semialtura 96 cm; cámara relativa Z=64 cm, unos 160 cm sobre suelo. No hay skeleton ni manos de producción.
- Ítaca: ARGOS en (250,-140,80) cm y escotilla en (620,0,80) cm son marcadores; no existe nave pilotable ni casco exterior. El destino de superficie es (0,1200,150) cm antes de ajustar al piso.
- `AstraeonCreatureActor` usa una esfera escalada para Umbra Grazer. Hay patrulla, alerta, amenaza y escaneo; no rig, animaciones ni cuadrúpedo modular implementado.
- `AstraeonWorldGenerator` genera una región acotada, recursos y POIs. El GameMode materializa cubos/rocas y plataformas. No se encontró un generador de planetas cube-sphere ni destrucción de terreno en runtime.
- Recursos reales: `silicate_fiber`, `ferrite_nodule` y un recurso característico elegido por seed. La receta real es `signal_resonator`; conservar IDs y datos C++ al cambiar presentación.
- Logs y estado registran smoke empaquetado exitoso, pero la escotilla falla manualmente. El código local ya contiene instrumentación `LogAstraeonInteract` que aún no figura como verificada en el estado. Esta entrega no certifica ese trabajo ajeno.

La guía planetaria especifica preparación futura de arte. Se preservan sus radios, topología y orden de pruebas, sin presentar esos sistemas como gameplay existente.

## Taxonomía, prioridad y dependencias

Clases: `MVP_BLOCKER` impide aceptar una capacidad jugable; `MVP_REQUIRED` necesita representación; `MVP_SUPPORTING` mejora lectura; `POST_MVP_FOUNDATION` sólo contrato futuro; `PHASE_2` producción de mundo vivo; `LATER` depende de expansiones congeladas. Riesgos T/A: técnico/artístico, B=bajo, M=medio, A=alto. Reutilización/procedural: baja, media o alta. «Previo UE» significa necesario antes de probar esa familia, no antes de probar el juego actual.

| Familia / alcance | Prioridad | Dependencia jugable | Reuso / procedural | T/A | Previo UE | Placeholder |
|---|---|---|---|---|---|---|
| Referencia humana y gálibo | MVP_SUPPORTING | Escala, AC-03 | alta / baja | B/B | Sí, escala | Sí |
| PlayerCharacter: cuerpo, cabeza, pelo, traje, botas, guantes, casco, mochila | MVP_SUPPORTING | Primera persona; cuerpo completo opcional | alta / media | A/A | No | Sí |
| Manos, escáner y herramienta recolectora | MVP_SUPPORTING | Lectura de escaneo/recolección | alta / media | M/M | No; HUD actual sirve | Sí |
| Protección básica y resonador | MVP_REQUIRED | Ambiente y receta AC-04/08 | media / media | B/M | No; inventario textual sirve | Sí |
| Ítaca: suelo, pared, techo, marco, hoja de escotilla | MVP_REQUIRED | Estancia AC-03 y transición | alta / alta | M/M | Sí, kit con escala | Sí |
| Consola ARGOS y acceso a bitácora | MVP_REQUIRED | Misión y registro | alta / media | M/M | Sí, blanco interactuable | Sí |
| Notas, títulos, fotos originales, luz y reparaciones | MVP_REQUIRED | Origen humano, narrativa mínima | alta / media | B/M | No | Sí, originales simples |
| Ítaca exterior: casco, motores, antena, tren, carga | POST_MVP_FOUNDATION | Ningún vuelo en MVP | alta / alta | A/A | No | Sí |
| Región: terreno y material de suelo legible | MVP_REQUIRED | Navegación AC-05 | alta / alta | M/M | Sí, suelo actual sirve | Sí |
| Rocas/landmarks y detalle regional Rocky | MVP_SUPPORTING | Orientación, ruta alternativa | alta / alta | M/M | No | Sí |
| Tres nodos de recurso, contenedor opcional | MVP_REQUIRED | Detectar/recolectar AC-07 | alta / alta | M/M | Sí, marcadores actuales sirven | Sí |
| Fuente de señal, barrera y anomalía menor | MVP_REQUIRED | AC-08/12, misterio | media / media | M/A | Sí, marcador sirve | Sí |
| Creature_Quadruped_A, Umbra Grazer y una variante | MVP_REQUIRED | AC-06 | alta / alta | A/A | Sí, familia antes de rig | Sí |
| HUD, visor, mapa, bitácora, estados de certeza e iconos | MVP_REQUIRED | AC-04/09/10 | alta / media | M/M | Sí, HUD actual sirve | Sí |
| Geometría de escaneo/holograma y alerta/daño | MVP_SUPPORTING | Feedback de herramientas y amenaza | alta / media | M/M | No | Sí |
| Cube-sphere de referencia, cuatro radios | POST_MVP_FOUNDATION | Validar escala futura, sin gameplay nuevo | alta / alta | M/B | Sí, TL_00 | Sí |
| Desert/Rocky/Forest/Ice/Volcanic/Saline: suelos, máscaras, props | PHASE_2 | Mundo vivo fuera del MVP | alta / alta | A/A | Sí, por TL | Sí |
| Props destructibles, soporte de fractura y regeneración | PHASE_2 | Requiere implementación y medición Chaos | alta / alta | A/M | Sí, cerrado/manifold | Sí |
| Otras familias: bípedos, voladores, reptadores, acuáticos/exóticos | PHASE_2 | Ecología y locomoción futuras | alta / alta | A/A | No hoy | Sí |
| NPC_Citizen/Scientist/Engineer/Trader, hostiles y narrativos | LATER | Primer contacto y sociedades | alta / alta | A/A | No hoy | Sí |
| Naves NPC, drones/sondas, cápsulas y pecios | LATER | Viaje y encuentros futuros | alta / alta | A/A | No hoy | Sí |
| Estructuras: corredores, estaciones, asentamientos, plataformas | LATER | Colonias/arquitectura; reusar kit humano | alta / alta | A/A | No hoy | Sí |
| Cuevas, ruinas ampliadas y tecnología alienígena | LATER | Exploración futura; sólo señal mínima hoy | alta / alta | A/A | No hoy | Sí |
| Maquinaria, extractores, almacenes, laboratorio/invernadero | LATER | Crafting/base avanzados | alta / alta | M/A | No hoy | Sí |
| Armas y equipamiento militar | LATER | Combate ampliado no obligatorio | alta / media | A/A | No hoy | Sí |
| VFX motores, escudos, destrucción ambiental | LATER | Sistemas futuros correspondientes | alta / media | A/A | No hoy | Sí |
| Portales | LATER, no autorizado | Sin requisito concreto encontrado | indeterminada | A/A | No | No producir |

Ninguna familia requiere arte final para este MVP. La falla de escotilla es `MVP_BLOCKER` funcional; no se resuelve generando una malla nueva. La representación de estancia y variante de criatura son pendientes de aceptación, aunque ya existan placeholders.

## Personaje: especificación técnica provisional

- Altura humana canónica de arte: 1,80 m con botas, ojos orientativos a 1,60 m. Proporciones humanas funcionales, cabeza ~1/7,5 de altura. Conservar cápsula actual de 1,92 × 0,84 m y validar gálibo; la referencia estática no reemplaza al Character.
- Skeleton futuro `SKEL_Humanoid_A`: root en suelo, pelvis, 3 segmentos de columna, cuello/cabeza; clavícula, brazo, antebrazo, mano por lado; muslo, pierna, pie y dedos por lado; 3 huesos por dedo. Huesos IK separados opcionales, jerarquía/nombres congelados después de una prueba de deformación y retarget.
- Propuesta de rig compartido para humano/NPC humanoide de proporciones compatibles. Ahorra animaciones, pero no imponerlo a anatomías alienígenas: requieren familia distinta. No declarar compatibilidad binaria con Manny/Quinn; primero probar importación e IK Retargeter con el skeleton realmente elegido. No descargar maniquíes ajenos.
- MVP: caminar, idle, salto/aterrizaje, mano escanea/interactúa; agachado sólo si entra en recorrido. Facial y pelo animado no requeridos; cabeza oculta en primera persona si se añade cuerpo.
- Sockets futuros: `socket_tool_r`, `socket_tool_l` en manos, `socket_backpack` en columna, `socket_helmet` en cabeza. Equipo en componentes separados, ropa comparte skin weights y pose; ocultar cuerpo bajo prendas para evitar clipping.
- Módulos: BaseBody, Head, Hair, SuitTorso, SuitLegs, Boots, Gloves, Helmet, Backpack, Equipment. Primero manos+traje visibles; no producir todas las combinaciones.
- Presupuesto provisional LOD0 cuerpo vestido ≤35k tris; manos FP ≤12k total; casco/mochila ≤5k cada uno. LOD1 ~50%, LOD2 ~25% para partes vistas a distancia; manos FP sin cambios perceptibles. ≤4 slots cuerpo y ≤2 manos; atlas 2K cuerpo, 2K manos y 1K equipamiento. Bloque humano actual: ≤144 tris, 1 material, sin textura ni skin.
- Colisión locomoción: cápsula C++; PhysicsAsset sólo al agregar skeletal mesh. No colisión por dedo ni simulación de tela en MVP.

## Ítaca: especificación y primer slice

Un ocupante, una estancia caminable. Módulo provisional de 8 × 6 × 2,8 m, grilla de 2 m, grosor de panel 0,12 m. Es un volumen de diseño; los cinco assets de esta entrega son una muestra de kit, no una habitación integrada.

Marco exterior 1,8 × 0,24 × 2,5 m; hueco 1,3 × 2,2 m deja 0,46 m de margen horizontal sobre cápsula y 0,28 m vertical. Consola 0,8 × 0,6 × 1,2 m. Ejes de los módulos: ancho X, profundidad Y, altura Z; frente de referencia -Y. Layout y conversión a ejes de Unreal deben comprobarse al montar.

MVP necesita entrada/transición controlada, consola de misión y bitácora (pueden compartir carcasa), señales del origen humano. No requiere cockpit pilotable, armas, almacenamiento físico, animación de tren ni motores funcionales. La escotilla puede conservar una transición sin animación; una hoja futura será un módulo separado con pivote apropiado.

Exterior provisional para futura exploración visual: envolvente de estudio 14 × 10 × 6 m, no canon aprobado ni asset producido. Hull/Engine/LandingGear/Antenna/Cargo se posponen hasta contar con requisitos de vuelo. Sin estados de daño, personalización o sistemas adicionales en MVP.

### Hallazgo de smoke manual post-integración (2026-09-05)

Con `AAstraeonItacaInterior` ya spawneada por `AAstraeonGameModeBase::BeginPlay` (paso 2/3 del orden de producción), el usuario jugó una sesión manual completa (ESCOTILLA, ARGOS, recolección, crafting, señal, save/close/open/continue: todo funcionó) y reportó dos observaciones puntuales sobre esta habitación, en sus propias palabras:

1. **"Al rededor de Ítaca hay como si fuera unas paredes"** y **"hay un pequeño problema con el piso de Ítaca... se sobrepisa con algún otro objeto como parte de esas paredes"**. Diagnóstico técnico (no corregido, para no pisar este trabajo): `AAstraeonItacaInterior` define su propio `FloorCollision` en Z de -12 a 0 dentro del mismo footprint donde `AAstraeonGameModeBase::MaterializeCurrentRegion` sigue generando el cubo procedural `Runtime_ProceduralRegionSurface` (Z de -100 a 0). Ambos pisos comparten la superficie superior en Z=0 → z-fighting/doble colisión en esa zona. Falta decidir una única fuente de verdad para el piso interior, o recortar el cubo procedural de región para respetar `AAstraeonItacaInterior::IsInsideFootprint`.
2. **Escotilla como puerta física (backlog, no urgente)**: el marco/hoja de escotilla no bloquea el paso — el jugador puede salir de la caja de Ítaca caminando por el hueco sin presionar `E`; `E` sólo dispara el teletransporte a la plataforma de superficie (`GetSurfaceDeploymentLocationCm`), que es una acción distinta de "cruzar el umbral". El usuario lo marcó explícitamente como "ahora o más adelante", no bloqueante.

Ver también `Docs/KNOWN_ISSUES.md` (mismas dos entradas, con más detalle técnico).

Primer slice elegido: **escala humana + kit de Ítaca**, porque desbloquea gálibo, interior, consolas y composición sin dependencia de rig ni generación planetaria. Assets: `SM_Ref_Human_180_Blockout`, suelo, pared, marco y consola ARGOS. La referencia humana tiene piezas rígidas y no es personaje animable.

## Familia de criatura

`Creature_Quadruped_A`: un torso, cuello/cabeza/mandíbula, cuatro cadenas de extremidades, pies y cola. Skeleton provisional root → pelvis/spine → cuello/cabeza/mandíbula; caderas/hombros y cadenas de tres articulaciones por miembro; cola de 4 segmentos. Interfaces en cuello, pelvis, hombros y raíz de cola; sockets para placas, cuernos y sensores. Priorizar un rig y pruebas de flexión antes de módulos intercambiables.

Umbra Grazer parte de una silueta baja y ancha, altura de hombro de estudio 0,7 m, longitud 1,4 m, consistente con el proxy observado; requiere revisión anatómica. Variante MVP por paleta y placas ligeras con el mismo esqueleto. Masa y daño siguen viniendo de gameplay, no del generador artístico.

Futuro contrato determinista: `EntitySeed + GeneratorVersion` → IDs de módulos + parámetros cuantizados + paleta, guardar deltas. Rangos iniciales para estudio: longitud torso 0,9–1,1, extremidades 0,95–1,05, cabeza 0,9–1,1. No escalar huesos independientemente sin pruebas de pies/IK. Animaciones mínimas: idle, marcha, alerta, amenaza y retirada; no matar obligatoriamente. ≤15k tris LOD0, 2 slots, atlas 2K; LOD1/2 50%/25%, cápsula o convexos simples. Rig, skin y variantes aún pendientes.

## Planetas, biomas y pruebas futuras

Contrato preservado: referencias cube-sphere, seis caras, 32 segmentos/33×33 vértices por cara, quads normalizados, normales hacia fuera, UV0 debug, origen central. Radios Small=150 m, Medium=400 m, Large=900 m, Giant=2000 m. 6144 quads / 12288 tris por referencia; soldar costuras si se necesita manifold (6146 vértices únicos), conservar 33×33 muestras lógicas por cara. No generar planetas runtime en Blender.

Cada bioma: dos capas tileables, tile de 4 × 4 m, BaseColor/Normal/ORM 2048²; Height opcional 1024²; DetailMask 1024² RGB macro/fino/borde. ORM=AO/Roughness/Metallic. Color sRGB, datos lineales. Ground triplanar, blending desde datos por vértice runtime; sin splat maps pintados por planeta.

Por bioma: 3 rocas pequeñas, 2 medianas, 1–2 grandes, 2–3 detalles primarios, 1–2 secundarios y 1 prop identitario. No forzar vegetación a ambientes incompatibles. Budgets LOD0 de la guía: pequeñas 150–400, medianas 500–1200, grandes 1500–4000, vegetación 400–1000, cards 20–80 tris. Destructibles medianos ≤800, grandes ≤2500, sólidos cerrados; no prefracturar ni prometer que el motor ya fractura. Empezar por Rocky regional cuando ayude al MVP, no por seis catálogos.

Orden preservado y todavía **no creado**: TL_00_ReferenceScale → TL_01_MaterialBiomeSingle → TL_02_BiomeBlend → TL_03_PropScatterSingleBiome → TL_04_AllBiomesOnePlanet → TL_05_PropDestruction → TL_06_TerrainDestructionRegen → TL_07_SizeTierSweep → TL_08_PerfStress. Bloquear cada nivel hasta pasar el anterior y disponer de su sistema C++.

Para MVP basta usar posteriormente un único mapa aislado `TL_Art_MVP` con estaciones escala/interior/criatura; no tres mapas separados. Inicialmente FBX roundtrip y preview Blender, después importación Unreal comprobando centímetros, orientación y colisión. El mapa jugable existente no se sobrescribe. Escala estática no demuestra transitabilidad ni interacción.

## Convenciones, calidad y orden de producción

Nombres: `SM_` estático, `SK_` skinned, `SKEL_` skeleton, `AN_` animación, `M_` maestro, `MI_` instancia, `T_` textura. Familia en inglés, labels de juego en español. Sufijo `_Blockout` obligatorio hasta revisión; `_Destructible` sólo tras certificación de geometría apropiada. Identificador estable en manifiesto, rutas y versión del generador, sin nombres aleatorios.

Calidad: Q0 referencia dimensional → Q1 blockout legible → Q2 prototipo integrado y medido → Q3 arte final (fuera del MVP). No ascender de nivel por sólo exportar. Módulos de interior Q1 ≤500 tris y 1–2 materiales; consola Q2 ≤3k, prop de misión ≤2k, atlas reutilizable 2K máximo inicial. Luz emissive no reemplaza iluminación. Instanciar materiales/mallas repetidos; medir 1080p en hardware objetivo antes de optimizar o subir budgets.

Orden con puertas de salida:

1. Auditoría y contratos → generador de kit → validación negativa/positiva → FBX individual y roundtrip → preview (esta entrega).
2. Importar kit en destino aislado de Unreal con escala 1, comprobar dimensiones en cm y gálibo/colisión de marco. Integración jugable después de diagnosticar escotilla; mantener proxies como fallback.
3. Montar estancia, conectar carcasas a marcadores sin cambiar IDs/progreso; smoke real ARGOS → escotilla. Añadir notas originales simples.
4. Tres recursos y señal/barrera: siluetas distinguibles, recolección y persistencia; validar receta.
5. Un cuadrúpedo con rig probado, marcha/alerta y variante; repetir AC-06.
6. Suelo Rocky y props regionales, UI/herramientas según lectura real; medir rendimiento y recorrido.
7. Referencias planetarias y ampliaciones sólo como trabajo posterior expresamente priorizado.

Evidencia por asset en `ContentPipeline/asset_manifest.json` y `ContentPipeline/reports/`. Un archivo fuente/FBX válido no certifica integración, licencia externa, animación ni rendimiento. Assets actuales originales generados localmente, sin descargas ni servicios de IA runtime.
