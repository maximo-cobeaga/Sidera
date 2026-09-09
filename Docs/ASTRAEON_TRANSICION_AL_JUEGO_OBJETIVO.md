---
title: "ASTRAEON — Plan maestro de transición al juego objetivo"
version: "1.0"
date: "2026-09-09"
status: "Dirección aprobada; pendiente de sincronización con los documentos rectores"
owner: "Máximo Andrés Cobeaga"
engine: "Unreal Engine 5.7.4"
dcc: "Blender 5.2.1"
ai_creation: "Higgsfield for Blender + Higgsfield Bridge"
platform: "Windows 11 x64"
mode: "Single-player offline primero"
performance_target: "1920x1080, 60 FPS, PC de gama media"
reference_hardware: "Intel i5-14400F, RTX 4060 8 GB, 16-32 GB RAM, SSD"
document_type: "Plan de transición, arquitectura, arte y producción"
---

# ASTRAEON — Plan maestro de transición al juego objetivo

## 0. Cómo debe utilizarse este documento

Este documento define cómo pasar del prototipo y del MVP original de **La primera señal** al juego objetivo de **ASTRAEON: Legado del Vacío**, sin construir sistemas que luego deban descartarse por asumir un mundo plano.

No es solamente una hoja de ruta de programación. Integra:

- Diseño jugable.
- Arquitectura de Unreal Engine.
- Generación procedural determinista.
- Planetas esféricos grandes.
- Gravedad radial.
- Streaming desde superficie hasta espacio.
- Producción visual.
- Creación asistida mediante Higgsfield.
- Limpieza, rigging, animación y exportación desde Blender.
- Presupuestos de rendimiento.
- Validación automática y visual.
- Reglas para agentes LLM.

Las palabras **DEBE**, **NO DEBE**, **DEBERÍA** y **PUEDE** se usan de forma normativa:

- **DEBE / NO DEBE:** requisito obligatorio.
- **DEBERÍA:** recomendación fuerte; desviarse requiere registrar el motivo.
- **PUEDE:** alternativa permitida.

El orden de las fases es obligatorio. Las fechas no lo son. Una fase termina por evidencia, no por cantidad de código o assets.

---

# 1. Decisión rectora

## 1.1 El planeta esférico deja de ser una expansión tardía

La siguiente decisión reemplaza la orientación anterior:

> Los planetas esféricos grandes, transitables y conectados de forma continua con el espacio son parte de la identidad central de ASTRAEON. Deben validarse antes de seguir produciendo sistemas de superficie dependientes de un mapa plano.

En consecuencia:

- La superficie jugable final DEBE pertenecer matemáticamente a un cuerpo esférico.
- Ningún sistema nuevo PUEDE asumir que el eje Z global representa arriba.
- El terreno final NO PUEDE basarse en Unreal Landscape plano.
- Una ciudad puede diseñarse temporalmente en un plano tangente, pero cada elemento DEBE proyectarse y orientarse sobre la esfera.
- El planeta NO DEBE ser una única esfera escalada ni una única malla estática.
- El tamaño visual del planeta NO DEBE resolverse aumentando Scale en un Blueprint.
- El mundo cercano DEBE materializarse mediante patches cargados por demanda.
- El jugador DEBE poder viajar entre superficie, atmósfera, órbita y espacio sin una pantalla de carga perceptible en el objetivo final.

## 1.2 Qué representa la esfera actual

El actor actual basado en una esfera escalada sirve únicamente para:

- Validar gravedad radial.
- Validar orientación del personaje.
- Probar cámara.
- Comprobar escala aparente.
- Experimentar con materiales atmosféricos u oceánicos.

No es el terreno final y no debe evolucionar mediante subdivisiones manuales del Static Mesh.

Su destino recomendado es convertirse en un banco de pruebas llamado:

~~~text
BP_PlanetGravityHarness
~~~

El planeta de gameplay será administrado por un runtime planetario en C++.

## 1.3 Relación con los documentos existentes

La dirección actual entra en conflicto con partes de:

- AGENTS.md.
- MVP_LA_PRIMERA_SENAL.md.
- GAME_DESIGN_MASTER.md.
- ASTRAEON_PLAN_DESARROLLO_MAPA.md.
- GUIA_ARTE_PLANETAS_BLENDER.md.

Este documento registra la nueva decisión, pero la **Fase 0** DEBE sincronizar esos archivos en un mismo cambio documental. Hasta hacerlo, un agente puede encontrar instrucciones incompatibles.

Cambios conceptuales requeridos:

| Documento actual | Regla anterior | Nueva interpretación |
|---|---|---|
| AGENTS.md | Planetas completos fuera del MVP | El contenido masivo sigue fuera, pero el núcleo esférico pasa a ser fundacional |
| MVP_LA_PRIMERA_SENAL.md | Una región puede existir sin planeta completo | La región se materializa sobre el sistema planetario esférico |
| GAME_DESIGN_MASTER.md | Sistema estelar en una fase posterior | El viaje completo sigue posterior, pero coordenadas, esfera y gravedad se adelantan |
| ASTRAEON_PLAN_DESARROLLO_MAPA.md | Región plana primero, planeta después | Cube-sphere, patches y gravedad radial pasan a las primeras fases |
| GUIA_ARTE_PLANETAS_BLENDER.md | Radios de 150 m a 2 km como tiers | Esos radios quedan como pruebas de laboratorio, no como planetas objetivo |

## 1.4 Qué se conserva

No se reinicia el proyecto. Se conservan todos los sistemas que no dependan de geometría plana:

- Seed y generación determinista.
- Ciencia ambiental.
- Escáner.
- Bitácora.
- Inventario y crafting.
- Guardado por seed más deltas.
- Interacciones.
- Criatura modular.
- Dirección artística.
- Ítaca, ARGOS y narrativa.
- Contratos de datos.
- Herramientas de Blender.
- Manifiestos de assets.

Se reemplazan o abstraen:

- Gravedad global fija.
- Movimiento que usa Z global.
- Coordenadas de región exclusivamente cartesianas.
- Colocación de props mediante una normal global.
- Sistemas que dependan directamente de Landscape.
- Navegación global plana.
- Guardados que persistan sólo un Transform mundial.

---

# 2. Definición del juego objetivo

La visión completa es demasiado grande para funcionar como una única condición de terminado. Se divide en tres productos verificables.

## 2.1 Producto A — Núcleo planetario

Demostración técnica sin contenido masivo:

- Un planeta cube-sphere cerrado.
- Radio configurable.
- Terreno radial determinista.
- Gravedad radial.
- Personaje capaz de recorrer cualquier orientación.
- Patches con LOD y colisión.
- Streaming sin grietas graves.
- Coordenadas locales estables.
- Océano y atmósfera provisionales.

Este producto responde una sola pregunta:

> ¿Puede ASTRAEON sostener un planeta grande real antes de construir el resto del juego?

## 2.2 Producto B — Vertical slice esférico

Reconstrucción de **La primera señal** sobre el núcleo planetario:

- Duración de 30 a 45 minutos.
- Interior mínimo de Ítaca.
- Descenso o llegada controlada.
- Región materializada sobre un planeta grande.
- Exploración, mediciones, escáner, mob, recursos y crafting.
- Hallazgo de la señal.
- Bitácora y guardado.
- Regeneración determinista.
- Sin dependencias de superficie plana.

Este producto valida que la tecnología planetaria puede sostener el bucle:

> explorar → medir → comprender → actuar → registrar

## 2.3 Producto C — Juego objetivo v1

Alcance mínimo recomendado para una primera versión comercial coherente:

- Un sistema estelar persistente.
- Un planeta principal grande.
- Una luna o segundo cuerpo explorable.
- La superficie completa de cada cuerpo objetivo es matemáticamente accesible, aunque la densidad de contenido varíe por región.
- Superficie, atmósfera, órbita y espacio conectados.
- Ítaca pilotable.
- Entre cuatro y seis biomas sólidos.
- Clima y peligros ambientales.
- Crafting, base y progresión de herramientas.
- Dos o tres familias de criaturas.
- Una especie inteligente.
- Una civilización y al menos un asentamiento modular significativo.
- Un arco narrativo completo alrededor de la primera señal.
- Simulación distante simplificada.
- Guardado robusto.
- Funcionamiento offline.

El universo procedural completo, múltiples civilizaciones, guerras extensas, genética, sucesión y expansión galáctica permanecen como evolución posterior. La arquitectura debe admitirlos, pero v1 no necesita agotarlos.

## 2.4 Visión de largo plazo

La visión completa conserva:

- Varios sistemas estelares.
- Planetas, lunas y estaciones.
- Biosferas causales.
- Civilizaciones e historia procedural.
- Idiomas, conocimiento y traducción.
- Diplomacia, ciudadanía y conflicto.
- Colonias y simulación remota.
- Muerte permanente y legado.
- Descendencia y genética validada.

La visión guía contratos; no habilita implementación prematura.

---

# 3. Restricciones globales

## 3.1 Producto

- Plataforma inicial: Windows 11 x64.
- Modalidad inicial: single-player offline.
- Perspectiva principal: primera persona.
- Tercera persona sólo si una fase posterior demuestra su valor.
- Sin IA generativa obligatoria durante el gameplay.
- Sin plugins pagos imprescindibles.
- El juego debe cargar una partida sin conexión.

## 3.2 Rendimiento

- Resolución de referencia: 1920 × 1080.
- Objetivo: 60 FPS.
- Frame budget total: 16,67 ms.
- Hardware de referencia: i5-14400F, RTX 4060 8 GB, 16–32 GB RAM y SSD.
- No aceptar stutter recurrente durante el movimiento rápido o el cambio de LOD.
- No generar colisión, vegetación o actores de gameplay para todo el planeta.
- Medir percentiles y picos; no depender solamente del promedio.

Objetivos iniciales de control:

| Métrica | Objetivo provisional |
|---|---:|
| FPS medio | 60 o más |
| Percentil 1% | 45 FPS o más |
| Hitch visible | Ningún pico recurrente superior a 50 ms |
| VRAM estable | Mantener margen en una GPU de 8 GB |
| Crecimiento de memoria | Sin crecimiento ilimitado en una sesión de 60 minutos |
| Generación | Fuera del game thread siempre que sea posible |

Estos valores se ajustan sólo con profiling registrado.

## 3.3 Arte

- Realismo legible y estilización moderada.
- No perseguir hiperrealismo que haga inviable la producción individual.
- Una silueta clara tiene prioridad sobre microdetalle.
- Texturas 2K por defecto.
- Texturas 4K sólo para assets hero justificados por captura y perfilado.
- Variación mediante módulos, morphs, materiales y parámetros; no mediante miles de assets independientes.

## 3.4 Inteligencia artificial de producción

Higgsfield se usa durante el desarrollo para:

- Concept art.
- Blockouts.
- Mallas iniciales.
- Rigs o animaciones iniciales.
- Texturas o referencias.
- Variantes visuales.
- Composición de escenas de prueba.

Higgsfield NO es fuente de verdad técnica. Toda salida debe atravesar Blender, validadores, manifiesto, exportación e importación en Unreal.

---

# 4. Arquitectura técnica objetivo

## 4.1 Pipeline causal

La arquitectura no es una muñeca rusa en la que UniverseGenerator controla todo. Es un DAG de módulos independientes:

~~~mermaid
flowchart TD
    Seed["Seed + versiones"] --> Planet["Definición planetaria"]
    Planet --> Climate["Clima y biomas"]
    Planet --> Surface["Superficie y patches"]
    Climate --> Content["Recursos y biosfera"]
    Surface --> Materialize["Materialización local"]
    Content --> Materialize
    Materialize --> Runtime["Gameplay + deltas"]
~~~

Cada módulo:

- Recibe entradas explícitas.
- Usa una seed derivada propia.
- Devuelve datos.
- No depende de orden accidental de threads.
- Puede probarse sin cargar el mundo completo.
- Tiene un validador.

## 4.2 Separar definición, representación y estado

### Definición

Hechos reproducibles:

- Radio.
- Masa.
- Gravedad.
- Atmósfera.
- Geología.
- Biomas.
- Seeds.

### Representación

Lo visible según distancia:

- Proxy orbital.
- Cube-sphere de bajo LOD.
- Patches detallados.
- Colisión cercana.
- Vegetación, edificios y criaturas.

### Estado mutable

Cambios de partida:

- Recursos recolectados.
- Deformaciones locales.
- Prop destruido.
- Puerta abierta.
- Criatura importante muerta.
- Ciudad dañada.
- Descubrimiento realizado.

El guardado persiste:

~~~text
Seed raíz
+ versiones de generadores
+ estado del jugador
+ deltas persistentes
~~~

No persiste millones de árboles ni cada vértice base.

## 4.3 Contratos mínimos

~~~cpp
struct FPlanetDefinition
{
    FGuid PlanetId;
    int64 PlanetSeed;
    int32 GeneratorVersion;

    double RadiusMeters;
    double MassKg;
    double SurfaceGravityMS2;
    double SeaLevelMeters;

    FAtmosphereDefinition Atmosphere;
    FGeologyDefinition Geology;
    FClimateDefinition Climate;
};

struct FPlanetPatchAddress
{
    FGuid PlanetId;
    uint8 FaceId;
    uint8 Lod;
    int32 X;
    int32 Y;

    bool operator==(const FPlanetPatchAddress& Other) const;
};

struct FPlanetPatchBuildResult
{
    FPlanetPatchAddress Address;
    int32 BuildRevision;
    TArray<FVector3f> LocalVertices;
    TArray<int32> Indices;
    TArray<FVector3f> Normals;
    TArray<FVector4f> BiomeWeights;
    FBox LocalBounds;
};

struct FLocalReferenceFrame
{
    FGuid CelestialBodyId;
    FVector3d OriginBodySpaceMeters;
    FQuat4d BodyToLocalRotation;
    int32 Revision;
};
~~~

## 4.4 Seeds estables

Nunca usar una secuencia aleatoria global para contenido persistente.

~~~cpp
PatchSeed = StableHash64(
    PlanetSeed,
    EGenerationChannel::Terrain,
    FaceId,
    Lod,
    PatchX,
    PatchY,
    TerrainGeneratorVersion
);
~~~

El hash debe estar definido por el proyecto y probado entre builds. El resultado no puede depender del orden de finalización de tareas asíncronas.

## 4.5 Cube-sphere por patches

Cada una de las seis caras usa coordenadas locales U y V entre -1 y 1.

~~~cpp
FVector3d CubePoint = FaceUvToCube(FaceId, U, V);
FVector3d Direction = CubePoint.GetSafeNormal();
double HeightMeters = TerrainHeight(PlanetSeed, Direction);

FVector3d BodySpacePosition =
    Direction * (PlanetRadiusMeters + HeightMeters);
~~~

Reglas:

- La altura se consulta mediante dirección planetaria global, no UV local aislada.
- Los bordes de dos caras deben consultar exactamente la misma función.
- Las normales deben derivarse con muestras coherentes a ambos lados del borde.
- Cada patch debe poder regenerarse de forma independiente.
- El grid inicial recomendado es 33 × 33 vértices por patch.
- Dos vecinos no deben diferir en más de un nivel de LOD.
- Usar skirts como solución inicial de grietas.
- Implementar stitching sólo después de medir que sea necesario.

## 4.6 LOD y streaming

Cada cara mantiene un quadtree. La decisión de subdivisión usa:

- Distancia al observador.
- Error geométrico proyectado en pantalla.
- Velocidad y dirección de viaje.
- Presencia de colisión o gameplay.
- Presupuesto máximo de patches.

Anillos de materialización:

| Anillo | Representación |
|---|---|
| Cercano | Malla detallada, colisión, gameplay, audio y props |
| Medio | Malla de detalle medio, props simplificados, sin IA completa |
| Lejano | Malla baja o HLOD, sin colisión |
| Orbital | Proxy planetario, océano, nubes y atmósfera |
| Remoto | Datos, icono o impostor; sin superficie cargada |

El runtime debe separar:

1. Solicitud.
2. Generación en CPU.
3. Validación.
4. Commit de malla en game thread.
5. Activación de colisión.
6. Materialización de contenido.

Una solicitud obsoleta debe poder cancelarse. Un resultado con revisión vieja no debe sobrescribir un patch nuevo.

## 4.7 Backend de malla

Unreal 5.7.4 marca ProceduralMeshComponent como experimental. Por eso el código no debe acoplar todo el planeta a una única implementación.

Crear una abstracción:

~~~cpp
class IPlanetPatchMeshBackend
{
public:
    virtual void CommitRenderMesh(const FPlanetPatchBuildResult& Build) = 0;
    virtual void CommitCollision(const FPlanetPatchBuildResult& Build) = 0;
    virtual void ReleasePatch(const FPlanetPatchAddress& Address) = 0;
};
~~~

Estrategia:

- Prototipo: ProceduralMeshComponent si acelera la prueba.
- Evaluación de producción: DynamicMeshComponent o backend C++ propio.
- Elegir después de comparar tiempo de commit, memoria, colisión y estabilidad.
- Registrar la decisión mediante ADR.

## 4.8 Gravedad radial

~~~cpp
FVector GravityDirection =
    (PlanetCenter - ActorLocation).GetSafeNormal();

CharacterMovement->SetGravityDirection(GravityDirection);
~~~

Unreal 5.7 expone dirección de gravedad personalizada en CharacterMovement, pero todavía deben resolverse:

- Orientación progresiva del capsule y mesh.
- Cámara sin roll involuntario.
- Salto.
- Falling.
- Movimiento sobre el plano tangente.
- Cambio entre cuerpos.
- Objetos físicos.
- Vehículos.
- IA.

El vector arriba local es:

~~~text
Up = Normalize(ActorPosition - PlanetCenter)
~~~

El movimiento se proyecta al plano tangente:

~~~cpp
Tangent = DesiredDirection
        - FVector::DotProduct(DesiredDirection, GravityDirection)
        * GravityDirection;
~~~

## 4.9 Coordenadas y precisión

Usar una jerarquía:

~~~text
Coordenada galáctica
└── sistema estelar
    └── cuerpo celeste
        └── patch
            └── frame local de gameplay
~~~

Reglas:

- Posiciones astronómicas y planetarias persistentes: double.
- Física, animación y render cercano: frame local.
- El Transform mundial nunca es la identidad persistente única de un objeto.
- Guardar BodyId, dirección superficial, altitud y coordenadas locales.
- Mantener un solo cuerpo gravitatorio activo para el gameplay cercano.
- Cambiar de frame en puntos controlados y probar la continuidad.

Large World Coordinates ayuda, pero no reemplaza marcos locales, streaming ni auditoría de conversiones float/double.

## 4.10 World Partition, PCG y planeta

Decisión recomendada:

- World Partition puede usarse para interiores, hubs artesanales y contenido persistente local.
- No debe ser el único streamer de la superficie esférica procedural.
- PlanetPatchManager controla topología, LOD, generación y colisión.
- PCG puede materializar vegetación y props dentro del frame local de un patch.
- ISM/HISM se usa para grandes cantidades de props repetidos.
- La generación PCG debe recibir puntos y normales ya convertidos al frame local.

## 4.11 Océano, atmósfera y nubes

- Océano: shell esférico con radio igual a Radius + SeaLevel.
- Costa: intersección entre elevación radial y nivel del mar.
- Atmósfera cercana: una solución de alta fidelidad para el cuerpo activo.
- Cuerpos distantes: material o shell simplificado.
- Nubes: capa esférica o volumen limitado al cuerpo activo.
- Toda transición orbital debe tener un fallback visual aunque la simulación detallada aún no esté lista.

## 4.12 Cuevas y destrucción

La superficie radial permite una altura por dirección; no representa bien cuevas, arcos o túneles.

Solución:

- Superficie global: heightfield radial.
- Cuevas: niveles o mallas modulares conectadas.
- Props destructibles: Chaos Geometry Collections.
- Terreno modificable: deltas locales por patch, almacenados como stamps o campos de desplazamiento.
- No fracturar toda la malla planetaria con Chaos.
- No implementar un planeta voxel completo durante las primeras fases.

---

# 5. Organización recomendada del repositorio

~~~text
Docs/
├── ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md
├── DEVELOPMENT_STATE.md
├── BACKLOG.md
├── TEST_REPORT.md
├── KNOWN_ISSUES.md
├── DECISIONS.md
├── PHASE_STATUS.md
└── ADR/

Source/Astraeon/
├── Core/
├── Planet/
│   ├── Definition/
│   ├── Coordinates/
│   ├── Gravity/
│   ├── Surface/
│   ├── Patches/
│   ├── LOD/
│   ├── Streaming/
│   ├── Climate/
│   └── Tests/
├── Space/
├── WorldGen/
├── Creatures/
├── Exploration/
├── Knowledge/
├── Persistence/
├── Narrative/
├── Civilizations/
└── Debug/

Content/Astraeon/
├── Data/
├── Player/
├── Ithaca/
├── Planets/
├── Biomes/
├── Creatures/
├── Civilizations/
├── Architecture/
├── Materials/
├── Maps/
└── Test/

ContentSource/Blender/
├── _Templates/
├── Player/
├── Ithaca/
├── Creatures/
├── Biomes/
├── Architecture/
├── Tools/
├── HF_Incoming/
└── Licenses/

Tools/Blender/
├── validate_scene.py
├── validate_asset.py
├── validate_family.py
├── generate_variants.py
├── render_turntable.py
├── export_unreal.py
└── presets/
~~~

Los fuentes .blend aprobados, texturas finales y FBX versionados deben seguir la política de Git LFS. Renders temporales, cachés y generaciones rechazadas no se confirman.

---

# 6. Resumen de fases

| Fase | Nombre | Resultado jugable | Puerta principal |
|---:|---|---|---|
| 0 | Transición controlada | Proyecto sin contradicciones rectoras | ADR y documentos sincronizados |
| 1 | Núcleo planetario | Caminar alrededor de una esfera real | Gravedad y cámara estables |
| 2 | Patches, LOD y precisión | Planeta grande recorrible técnicamente | Streaming sin grietas ni hitches recurrentes |
| 3 | La primera señal esférica | Vertical slice completo sobre el planeta | Recorrido de 30–45 min y guardado |
| 4 | Planeta visual y biomas | Mundo reconocible desde suelo y órbita | Seis biomas y rendimiento aprobado |
| 5 | Protagonista, Ítaca y vuelo | Despegue, órbita y aterrizaje | Ida y vuelta sin carga perceptible |
| 6 | Sistema estelar | Viaje a una luna o segundo cuerpo | Persistencia entre cuerpos |
| 7 | Mundo vivo | Ecología, clima y progresión | Sistemas causales y estables |
| 8 | Primer contacto | Especie, civilización y ciudad | Ciudad coherente, transitable y narrativa |
| 9 | Sociedad y legado | Diplomacia, colonia y sucesión | Consecuencias persistentes |
| 10 | Expansión y lanzamiento | Juego objetivo v1 | Build estable, medido y testeado |

---

# 7. Fases detalladas

## Fase 0 — Transición controlada

### Objetivo

Cambiar la base planetaria sin perder sistemas válidos ni dejar instrucciones contradictorias.

### Desarrollo

- Crear un ADR para planetas esféricos como requisito fundacional.
- Actualizar AGENTS.md, MVP, documento maestro y plan de mapa.
- Identificar código que use Z global, gravedad global o coordenadas planas.
- Clasificar cada sistema como reusable, adaptable o descartable.
- Etiquetar una build base antes del cambio.
- Registrar métricas actuales.
- Crear PlanetLab separado del mapa jugable.

### Blender e Higgsfield

- Congelar una plantilla Blender aprobada.
- Registrar versión del add-on y de Blender.
- Ejecutar un smoke test de Higgsfield Bridge.
- Crear una escena de calibración con cubo de 1 m, mannequin de 1,80 m y ejes.
- Validar una importación estática y una esquelética en Unreal.

### Entregables

- ADR aprobado.
- Matriz de migración.
- TL_00_AssetCalibration.
- PlanetLab vacío.
- Baseline de rendimiento.
- Checklist de versiones.

### Puerta de salida

- No quedan documentos rectores que indiquen construir un Landscape plano.
- El proyecto compila.
- La calibración Blender → Unreal respeta escala y ejes.
- Existe una build recuperable anterior a la transición.

### Fuera de alcance

- Biomas finales.
- Nave pilotable.
- Ciudades.
- Generación masiva de assets.

## Fase 1 — Núcleo planetario

### Objetivo

Demostrar una esfera procedural cerrada y una locomoción radial estable.

### Desarrollo

- Implementar PlanetDefinition.
- Crear seis caras cube-sphere de baja resolución.
- Implementar altura radial determinista simple.
- Implementar gravedad radial mediante un componente.
- Alinear capsule, mesh y cámara.
- Implementar caminar, correr, saltar y agacharse.
- Cruzar los seis bordes de cara.
- Crear pruebas de seed y de continuidad.

Tiers de prueba de ingeniería:

| Tier | Radio | Propósito |
|---|---:|---|
| Lab | 10 km | Depuración rápida |
| Target | 500 km | Prueba principal de escala |
| Stress | 2500 km | Precisión y cambio de representación |

Estos tiers no fijan el canon; prueban que el tamaño sea un dato y no una escala manual.

### Blender e Higgsfield

- No modelar el planeta final.
- Crear solamente:
  - Material checker triplanar.
  - Marcadores de eje y polos.
  - Props de escala.
  - Mannequin provisional.
- Higgsfield puede generar referencias visuales de superficie, no topología planetaria.

### Entregables

- APlanetRuntime.
- UPlanetGravityComponent.
- CubeSphereTopology.
- TL_10_RadialGravity.
- TL_11_CubeSphereClosed.
- Overlay de centro, dirección, cara y coordenadas.

### Puerta de salida

- El personaje completa una vuelta lógica sin perder orientación.
- No existe costura abierta entre caras.
- Saltar y caer funciona en polos, ecuador y bordes.
- La cámara no gira bruscamente.
- Dos ejecuciones con la misma seed generan los mismos valores.

## Fase 2 — Patches, LOD, colisión y precisión

### Objetivo

Convertir la esfera completa en un planeta grande que sólo materializa detalle cercano.

### Desarrollo

- Dividir cada cara en quadtree.
- Crear FPlanetPatchAddress.
- Generar patches en workers.
- Commit controlado en game thread.
- Implementar cancelación y BuildRevision.
- Agregar skirts.
- Limitar diferencia de LOD entre vecinos.
- Activar colisión sólo en anillos cercanos.
- Implementar LocalReferenceFrame.
- Probar transición entre frames.
- Agregar persistencia de dirección planetaria y altitud.
- Implementar proxy orbital.

### Blender e Higgsfield

- Crear siluetas y mapas de referencia para montañas.
- Generar con Higgsfield conceptos de formaciones; convertir sólo las seleccionadas en stamps matemáticos o props.
- Crear tres landmarks hero para probar lectura a distancia.
- Mantener cada salida en HF_Incoming hasta validación.

### Entregables

- PlanetPatchManager.
- PlanetLODManager.
- PlanetStreamingManager.
- LocalReferenceFrameManager.
- TL_12_PatchLOD.
- TL_13_CollisionRing.
- TL_14_FrameTransition.
- Perfil en Unreal Insights.

### Puerta de salida

- El tier Target de 500 km funciona sin que la cantidad de patches activos de alta resolución crezca linealmente con el radio.
- El stress test no produce jitter visible cerca del jugador.
- No hay grietas visibles desde la ruta de prueba.
- La colisión no desaparece bajo el jugador.
- Ir y volver regenera el mismo patch.
- No hay hitches recurrentes superiores al presupuesto acordado.

## Fase 3 — La primera señal esférica

### Objetivo

Portar el vertical slice completo al planeta definitivo.

### Desarrollo

- Convertir RegionId a región planetaria.
- Proyectar spawn, recursos, señal y criatura.
- Adaptar escáner a coordenadas planetarias.
- Adaptar mapa y bitácora.
- Guardar BodyId, patch, dirección y altitud.
- Materializar rutas críticas antes que decoración.
- Validar transitabilidad sobre la esfera.
- Mantener una llegada controlada mientras el vuelo libre no exista.

### Blender e Higgsfield

- Completar Creature_Quadruped_A.
- Crear ruina o emisor de la señal.
- Crear tres recursos.
- Crear herramienta de escaneo y recolección.
- Crear kit visual mínimo de Ítaca.
- Generar turntables y variantes antes de aprobar assets.

### Entregables

- Build jugable de 30–45 minutos.
- Una región esférica coherente.
- Mob integrado.
- Recursos, crafting, señal, bitácora y guardado.
- Reporte de tres recorridos completos.

### Puerta de salida

- Todos los criterios jugables originales del MVP pasan sobre la esfera.
- Guardar, cerrar, abrir y continuar funciona.
- La misma seed conserva terreno y contenido esencial.
- No existe dependencia funcional de Landscape.
- La build mantiene el presupuesto de rendimiento.

## Fase 4 — Planeta visual, clima y biomas

### Objetivo

Hacer que el planeta posea identidad desde el suelo, el aire y la órbita.

### Desarrollo

- Separar macrorelieve y microdetalle.
- Generar continentes, cordilleras, cráteres y cuencas.
- Implementar temperatura, humedad y altura.
- Calcular pesos de bioma por vértice o datos equivalentes.
- Crear material triplanar.
- Agregar océano esférico.
- Agregar atmósfera y nubes.
- Materializar props mediante PCG o HISM dentro del frame local.
- Implementar deltas locales de terreno.

Orden causal:

~~~text
Geología
→ macrorelieve
→ nivel del mar y drenaje
→ clima
→ biomas
→ recursos
→ props y vida
~~~

### Blender e Higgsfield

Producir seis kits:

- Desert.
- Rocky.
- Forest.
- Ice.
- Volcanic.
- Saline.

Por bioma:

- Dos superficies tileables.
- DetailMask.
- Tres rocas pequeñas.
- Dos rocas medianas.
- Una o dos formaciones.
- Dos o tres props primarios.
- Uno o dos props secundarios.
- Un landmark.

Higgsfield propone diseños y mallas iniciales. Blender corrige escala, topología, pivote, UV, materiales, colisión y LOD.

### Entregables

- TL_20_SingleBiome.
- TL_21_BiomeBlend.
- TL_22_AllBiomes.
- TL_23_OrbitReadability.
- TL_24_PropStress.
- Catálogos y manifiestos de seis biomas.

### Puerta de salida

- No hay costuras de material evidentes entre caras.
- Cada bioma se reconoce sin leer su nombre.
- La identidad visual tiene una causa ambiental.
- La órbita conserva continentes y landmarks.
- El scatter no rompe rutas ni colisión.
- El planeta cumple el presupuesto de GPU y VRAM.

## Fase 5 — Protagonista, Ítaca y vuelo continuo

### Objetivo

Dar identidad al protagonista y conectar físicamente suelo, nave, atmósfera y órbita.

### Desarrollo

- Crear estados de nave.
- Implementar entrada, salida y asiento.
- Implementar controles atmosféricos y orbitales iniciales.
- Precargar patches de aterrizaje.
- Cambiar representación planetaria sin pantalla de carga.
- Resolver gravedad de nave y personaje.
- Mantener cámara estable durante transiciones.

Estados:

~~~text
Docked
→ Launching
→ AtmosphericFlight
→ Orbit
→ PlanetApproach
→ AtmosphericEntry
→ Landing
→ Docked
~~~

### Blender e Higgsfield — protagonista

Crear:

- Personaje con casco.
- Personaje sin casco.
- Traje base modular.
- Guantes y botas.
- Mochila o soporte vital.
- Puntos de anclaje para armas, herramientas y objetos.
- Brazos de primera persona.
- Cuerpo completo para sombras, reflejos y escenas futuras.

Animaciones mínimas:

- Idle.
- Walk.
- Run.
- Jump_Start.
- Jump_Loop.
- Jump_Land.
- Crouch_Idle.
- Crouch_Walk.
- Equip.
- Unequip.
- Hold_Rifle.
- Hold_Tool.
- Interact.

### Blender e Higgsfield — Ítaca

Separar:

- Exterior de vuelo.
- Cockpit.
- Interior mínimo.
- Puertas y rampas.
- Tren de aterrizaje.
- Motores y VFX sockets.
- Daño modular.
- LOD orbital.

El exterior y el interior no necesitan ser una única malla.

### Entregables

- Familia modular Player_Human_A.
- Kit Ithaca_Exterior_A.
- Kit Ithaca_Interior_A.
- AnimBP y sockets.
- TL_30_PlayerAnimation.
- TL_31_IthacaInterior.
- TL_32_SurfaceToOrbit.

### Puerta de salida

- El personaje sostiene herramientas sin atravesar manos de forma grave.
- Todas las animaciones usan el esqueleto canónico.
- El jugador despega, entra en órbita y aterriza.
- La ubicación del planeta persiste.
- No aparece una pantalla de carga perceptible.
- La transición mantiene rendimiento y control.

## Fase 6 — Sistema estelar

### Objetivo

Viajar a una luna o segundo cuerpo sin romper precisión ni persistencia.

### Desarrollo

- Crear SolarSystemDefinition.
- Implementar cuerpos y órbitas como datos.
- Crear proxies remotos.
- Implementar navegación y selección de destino.
- Cambiar cuerpo activo y frame local.
- Simular viaje a nivel apropiado.
- Descargar la superficie del origen.
- Precargar el destino.

### Blender e Higgsfield

- Crear variantes de roca espacial.
- Crear estación o baliza de navegación.
- Crear shaders y siluetas para cuerpos remotos.
- Crear assets de cockpit relacionados con navegación.

### Puerta de salida

- Viaje ida y vuelta entre dos cuerpos.
- Estado persistente en ambos.
- Sin objetos duplicados.
- Sin pérdida de precisión.
- La representación remota coincide con la superficie generada.

## Fase 7 — Mundo vivo

### Objetivo

Agregar ecología, clima y progresión sin perder estabilidad planetaria.

### Desarrollo

- Clima dinámico local.
- Cadenas ecológicas simplificadas.
- Migración y necesidades de criaturas.
- Recursos renovables y no renovables.
- Base modular.
- Farming en entorno controlado.
- Peligros científicos más profundos.
- Simulación remota agregada.

### Blender e Higgsfield

- Ampliar Creature_Quadruped_A.
- Crear Creature_Flying_A o Creature_Insectoid_A.
- Crear vegetación modular.
- Crear piezas de base.
- Crear daños, edad y estados ambientales.

No iniciar una familia completa hasta que un ejemplar pase:

~~~text
concepto
→ game mesh
→ rig
→ animación
→ material
→ LOD
→ importación
→ comportamiento
→ profiling
~~~

### Puerta de salida

- Las criaturas tienen nicho y no sólo apariencia.
- El clima modifica decisiones.
- La base apoya exploración.
- La simulación distante no intenta ejecutar cada individuo.
- Cien variantes deterministas pasan validación.

## Fase 8 — Primer contacto, civilización y ciudad

### Objetivo

Demostrar que biología, cultura, historia y arquitectura pueden producir una sociedad coherente.

### Desarrollo

- Crear SpeciesDefinition.
- Crear CivilizationDefinition.
- Simular historia local.
- Generar un asentamiento.
- Generar distritos y rutas.
- Proyectar la ciudad por sectores sobre la esfera.
- Crear navegación local.
- Implementar idioma y traducción inicial.
- Relacionar evidencias físicas con eventos históricos.

### Blender e Higgsfield

- Crear Citizen_Humanoid_A o la familia elegida.
- Crear kit arquitectónico de una civilización.
- Crear ropa por capas.
- Crear símbolos y señalética.
- Crear herramientas y tecnología.
- Crear ruinas y estados de daño.

La ciudad se diseña en planos tangentes por distrito y se proyecta:

~~~text
coordenada urbana
→ dirección planetaria
→ altura
→ posición sobre superficie
→ orientación según normal local
~~~

### Puerta de salida

- La ciudad tiene entradas, rutas y distritos transitables.
- Su arquitectura expresa ambiente, tecnología e historia.
- El jugador descubre historia mediante el escenario.
- Dos seeds producen variación sin combinaciones absurdas.
- NPC y navegación funcionan dentro del área activa.

## Fase 9 — Sociedad persistente y legado

### Objetivo

Agregar consecuencias de largo plazo sin convertir ASTRAEON en un city builder.

### Desarrollo

- Ciudadanía y legitimidad.
- Diplomacia.
- Economía agregada.
- Colonias.
- Órdenes remotas.
- Conflicto.
- Muerte permanente.
- Sucesión.
- Herencia de conocimiento y relaciones.

### Blender e Higgsfield

- Uniformes y estados sociales.
- Arquitectura próspera, dañada, ocupada y abandonada.
- Variantes de edad.
- Equipamiento profesional.
- Monumentos y memoriales.

### Puerta de salida

- Una acción del jugador produce una consecuencia persistente observable.
- El mundo continúa tras una muerte.
- El sucesor hereda sólo lo definido.
- La estrategia nace de acciones y decisiones en primera persona.

## Fase 10 — Expansión y lanzamiento

### Objetivo

Convertir el sistema probado en una versión publicable.

### Desarrollo

- Varios sistemas mediante generación diferida.
- Optimización.
- Accesibilidad.
- Controles configurables.
- Compatibilidad de saves.
- Telemetría local opcional y respetuosa.
- Pruebas de larga duración.
- Empaquetado y actualización.

### Blender e Higgsfield

- Sólo ampliar familias cuya métrica de variedad lo justifique.
- Sustituir placeholders visibles.
- Optimizar atlas, materiales y LOD.
- Crear material promocional separado de los assets de gameplay.

### Puerta de salida

- Build limpia desde checkout documentado.
- Cero crashes conocidos en la ruta crítica.
- Tres sesiones completas consecutivas.
- Rendimiento validado en hardware de referencia.
- Guardados compatibles o migrables.
- Licencias y procedencia registradas.
- Lista explícita de limitaciones.

---

# 8. Pipeline Higgsfield → Blender → Unreal

## 8.1 Principio

Higgsfield acelera la propuesta y la primera construcción. Blender convierte la propuesta en un asset controlado. Unreal decide si el asset sirve para gameplay.

~~~text
Brief
→ conceptos
→ selección
→ generación Higgsfield
→ cuarentena HF_Incoming
→ limpieza Blender
→ retopología/modularización
→ UV y materiales
→ rig/animación
→ LOD y colisión
→ validación
→ exportación
→ importación Unreal
→ prueba jugable
→ aprobación
~~~

Ninguna generación salta etapas.

## 8.2 Compatibilidad de versiones

Estado al redactar este documento:

- Blender local: 5.2.1 LTS.
- Unreal: 5.7.4.
- El smoke test local de Higgsfield fue satisfactorio.
- La página oficial de Higgsfield declara soporte para Blender 4.2 a 5.1.

Por lo tanto, Blender 5.2.1 se considera **compatible de forma provisional en este proyecto**, no oficialmente garantizado.

Después de cada actualización de Blender o Higgsfield:

1. Abrir la escena de calibración.
2. Conectar el Bridge.
3. Crear un objeto de prueba dentro de HF_Incoming.
4. Guardar y reabrir.
5. Ejecutar validación headless.
6. Exportar.
7. Importar en Unreal.
8. Registrar resultado y versiones.

Si falla, mantener una instalación compatible en paralelo. No actualizar el entorno de producción a mitad de una familia de assets.

## 8.3 Estructura de una escena Blender

Cada archivo aprobado usa colecciones:

~~~text
00_REFERENCE
10_HF_INCOMING
20_BLOCKOUT
30_HIGHPOLY
40_GAME_MESH
50_RIG
60_COLLISION
70_EXPORT
90_ARCHIVE
~~~

Reglas:

- Higgsfield sólo crea o modifica objetos dentro de 10_HF_INCOMING, salvo instrucción explícita.
- 00_REFERENCE nunca se exporta.
- 70_EXPORT contiene únicamente objetos aprobados.
- La fuente aprobada nunca se sobrescribe con una regeneración.
- Cada iteración relevante se guarda como versión recuperable.

## 8.4 Brief obligatorio

Antes de generar, crear un archivo textual:

~~~yaml
asset_id: SM_Rock_Volcanic_Large_001
asset_family: Biome_Volcanic
purpose: landmark_navigational
dimensions_m: [6.0, 4.0, 8.0]
pivot: base_center
style: readable_scifi_realism
biological_or_material_cause: basaltic_column_erosion
target_lod0_triangles: 3500
uv_strategy: unique_plus_tileable_detail
collision: custom_convex
nanite_candidate: true
required_variants: 1
forbidden:
  - floating_parts
  - extreme_overhang_without_support
  - copied_franchise_language
status: brief
~~~

Un asset sin brief no entra al pipeline.

## 8.5 Prompt base para Higgsfield Bridge

~~~text
Trabajá únicamente dentro de la colección 10_HF_INCOMING de la escena abierta.
No borres, renombres ni modifiques objetos de otras colecciones.

Creá [ASSET] para ASTRAEON, un juego sci-fi de realismo legible.
Función jugable: [FUNCIÓN].
Dimensiones reales: [X, Y, Z] metros.
Dirección artística: [DESCRIPCIÓN].
Causa física o biológica: [CAUSA].

Entregá un blockout editable con:
- silueta clara;
- piezas separadas según [MÓDULOS];
- origen provisional en la base;
- transformaciones identificables;
- sin detalles microscópicos;
- sin texto ni logos;
- sin elementos inspirados directamente en franquicias reconocibles.

No lo marques como final, no lo exportes y no modifiques el rig canónico.
Nombrá la colección generada WIP_HF_[ASSET_ID]_[VARIANTE].
~~~

## 8.6 Prompt para criatura o personaje

~~~text
Usá el brief y el contrato anatómico [FAMILY_ID].
Conservá exactamente la bind pose, cantidad de extremidades, conectores y
esqueleto canónico. Generá sólo [MÓDULO O CUERPO BASE].

Debe soportar:
- locomoción;
- lectura de silueta a distancia;
- deformación en las articulaciones;
- conexión con [SLOTS];
- material biológico paramétrico;
- retopología posterior.

No agregues huesos, no cambies nombres, no combines el mesh con accesorios y
no produzcas una pose dramática. El resultado debe quedar en bind pose dentro
de 10_HF_INCOMING.
~~~

## 8.7 Prompt para animación

~~~text
Animá exclusivamente el armature [SKELETON_ID].
No cambies jerarquía, nombres, bind pose ni escala.
Creá una acción separada llamada [ACTION_NAME].

Movimiento: [DESCRIPCIÓN].
Duración objetivo: [SEGUNDOS].
Loop: [SÍ/NO].
Root motion: [IN_PLACE/ROOT_MOTION].
Contacto obligatorio: [PIES/MANOS/OBJETO].

Limpiá deslizamiento de pies, conservá el centro de masa y dejá keyframes
editables. No mezcles varias acciones en una sola timeline.
~~~

Política:

- Locomoción normal: in-place.
- Ataques, montajes y movimientos especiales: root motion sólo si se justifica.
- Una Action por clip.
- Simplify de exportación en 0.
- Revisar pies, manos, arcos y centro de masa.

## 8.8 Prompt para Ítaca o arquitectura modular

~~~text
Creá un blockout modular, no una nave o edificio fusionado.
Usá una cuadrícula de 1 metro y conectores de [TAMAÑO].
Separá exterior, interior, puertas, rampas, paneles y piezas repetibles.

Cada módulo debe:
- poder reemplazarse;
- tener una función;
- poseer espesor plausible;
- admitir colisión simple;
- evitar pasillos menores al ancho definido;
- respetar silueta y lenguaje visual de ASTRAEON.

No modeles cableado fino, tornillos ni daño final en esta etapa.
~~~

## 8.9 Limpieza en Blender

Checklist geométrico:

- Unidades métricas.
- Unit Scale = 1.0.
- Dimensiones reales.
- Rotation y Scale aplicadas.
- Normales hacia afuera.
- Sin vértices duplicados accidentales.
- Sin caras degeneradas.
- Sin ngons en game mesh.
- Manifold cuando sea obligatorio.
- Pivote correcto.
- Nombres aprobados.
- Material slots mínimos.
- Densidad de texel coherente.
- UV sin solapamientos accidentales.

Una malla de IA defectuosa puede:

- Servir como concepto.
- Servir como high poly para baking.
- Ser retopologizada.
- Ser regenerada.

No debe repararse indefinidamente. Si la limpieza cuesta más que una nueva generación controlada, se descarta.

## 8.10 Rigging

Por familia:

- Un esqueleto.
- Una bind pose.
- Nombres inmutables.
- Root bone estable.
- Huesos IK documentados.
- Sockets documentados.
- Pesos normalizados.
- Sin influencias huérfanas.
- Pruebas en poses extremas.

Para el protagonista:

- Skeleton_Player_Human_A.
- Socket_Hand_R.
- Socket_Hand_L.
- Socket_Back.
- Socket_Hip.
- Socket_Tool.
- Socket_Weapon.

Para Creature_Quadruped_A se conserva el contrato de conectores existente.

## 8.11 Materiales y texturas

Convención:

- BaseColor: sRGB.
- Emissive: sRGB salvo razón técnica.
- Normal: lineal, no sRGB.
- ORM: lineal, no sRGB.
- Height: lineal, no sRGB.
- Masks: lineal, no sRGB.

Empaquetado ORM:

~~~text
R = Ambient Occlusion
G = Roughness
B = Metallic
~~~

Reglas:

- 2K por defecto.
- 1K para máscaras secundarias.
- 4K sólo por excepción medida.
- Material maestro por familia.
- Variantes mediante Material Instances.
- Texturas tileables para terreno.
- Triplanar para superficie planetaria.

## 8.12 LOD, Nanite y colisión

- Nanite es candidato para estáticos complejos; no una excusa para ignorar material slots, memoria o overdraw.
- Medir Nanite por categoría antes de activarlo masivamente.
- Vegetación repetida usa instancing y estrategia específica.
- Skeletal meshes mantienen LOD explícitos.
- Props pequeños deben tener colisión simple o ninguna.
- Edificios, rampas y puertas requieren colisión diseñada.
- Criaturas usan Physics Asset; no colisión per-poly.
- Props destructibles deben ser cerrados y manifold.

## 8.13 Exportación

Preset del proyecto:

~~~text
Nombre: UE57_AST_V1
Formato: FBX
Export: Selected Objects
Forward: -Y
Up: Z
Global Scale: 1.0
Apply Unit: enabled
Add Leaf Bones: disabled
Bake Animation: según asset
Simplify: 0.0 para animación
~~~

Importación Unreal:

- Import Uniform Scale = 1.0.
- Elegir Skeleton existente cuando corresponde.
- Import Morph Targets sólo cuando el test de morphs esté aprobado.
- No depender del material automático para materiales complejos.
- Verificar normals, tangents, escala, pivote, colisión y sockets.

La tubería FBX de Unreal usa FBX 2020.2. Blender no expone exactamente la misma selección de versión que Autodesk; por eso el proyecto necesita un test de ida y vuelta y un preset congelado.

Antes de producir morphs en masa:

1. Crear un cubo o cabeza con un shape key.
2. Exportar mediante UE57_AST_V1.
3. Importar con Morph Targets.
4. Confirmar nombre y deformación.
5. Reimportar.
6. Automatizar la regresión.

## 8.14 Estados del asset

~~~text
brief
→ generated
→ triaged
→ cleaned
→ game_ready
→ exported
→ imported
→ validated
→ approved
~~~

Sólo approved puede aparecer en un mapa de producción.

---

# 9. Contratos por categoría de asset

## 9.1 Planeta

Blender produce:

- Referencias.
- Materiales.
- Props.
- Landmarks.
- Shells visuales de prueba.

Blender no produce:

- La malla completa final.
- Los patches runtime.
- La distribución definitiva.
- El LOD planetario.

## 9.2 Props

| Categoría | LOD0 orientativo |
|---|---:|
| Roca pequeña | 150–400 tris |
| Roca mediana | 500–1200 tris |
| Formación grande | 1500–4000 tris |
| Vegetación individual | 400–1000 tris |
| Card de vegetación | 20–80 tris |

Pivote en base, XY centrado, Z = 0. No depender de una vista única.

## 9.3 Criaturas y ciudadanos

| Uso | LOD0 | LOD1 | LOD2 | LOD3 |
|---|---:|---:|---:|---:|
| Mob principal cercano | ≤ 80k | ≤ 40k | ≤ 16k | ≤ 5k |
| Ciudadano cercano | ≤ 100k conjunto | ≤ 50k | ≤ 20k | ≤ 6k |
| Individuo de fondo | No usar LOD0 | ≤ 30k | ≤ 12k | ≤ 4k |

El límite cuenta el ensamblado completo.

## 9.4 Protagonista

- Cuerpo completo y brazos de primera persona pueden compartir lenguaje visual, pero tienen necesidades de cámara distintas.
- Casco y cabeza deben ser intercambiables.
- El traje debe admitir daños y módulos.
- Manos deben probarse con todos los grips.
- No aprobar el personaje por render; aprobarlo animado en gameplay.

## 9.5 Ítaca

- Exterior, interior y cockpit separados.
- Módulos repetibles.
- Interiores con escala humana verificada.
- Puertas y rampas como piezas funcionales.
- Colisión simple.
- LOD orbital separado.
- Sockets de motor, luces, armas y tren de aterrizaje.

## 9.6 Arquitectura

- Cuadrícula de 1 m.
- Conectores versionados.
- Pisos, muros, techos, puertas y esquinas separados.
- Anchura transitable verificada con capsule.
- Pivotes y sockets consistentes.
- Estado normal, dañado y abandonado por composición, no por duplicar todo el kit.

---

# 10. Automatización y validación

## 10.1 Scripts mínimos de Blender

- validate_scene.py.
- validate_asset.py.
- validate_family.py.
- generate_variants.py.
- render_turntable.py.
- export_unreal.py.

Ejemplo PowerShell:

~~~powershell
$AstraeonBlender = "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"

& $AstraeonBlender --background ".\ContentSource\Blender\Creatures\Creature_Quadruped_A.blend" --python ".\Tools\Blender\validate_family.py" -- --family "Creature_Quadruped_A" --manifest ".\Tools\Blender\presets\Creature_Quadruped_A.json"
~~~

Todos los scripts:

- Aceptan argumentos.
- Escriben logs.
- Devuelven exit code no cero ante fallo.
- Son deterministas cuando aplica.
- No pisan fuentes aprobadas.

## 10.2 Pruebas Unreal

Mínimo:

- Misma seed, mismo planeta.
- Distinta seed, variación medible.
- Continuidad de bordes.
- Vecinos con LOD compatible.
- Gravedad en puntos cardinales.
- Salto y caída.
- Cambio de frame.
- Unload y reload de patch.
- Persistencia de deltas.
- Importación de asset.
- Morph target.
- Animaciones y sockets.
- Build empaquetada.

## 10.3 Test levels

~~~text
TL_00_AssetCalibration
TL_10_RadialGravity
TL_11_CubeSphereClosed
TL_12_PatchLOD
TL_13_CollisionRing
TL_14_FrameTransition
TL_20_SingleBiome
TL_21_BiomeBlend
TL_22_AllBiomes
TL_23_OrbitReadability
TL_24_PropStress
TL_30_PlayerAnimation
TL_31_IthacaInterior
TL_32_SurfaceToOrbit
TL_40_TwoBodyTravel
~~~

## 10.4 Evidencia por fase

Cada puerta necesita:

- Commit o diff identificable.
- Comando de build.
- Resultado de tests.
- Captura o video.
- Perfil de rendimiento.
- Lista de warnings nuevos.
- Limitaciones conocidas.
- Próximo paso.

Una captura bonita no reemplaza una prueba. Una prueba verde no reemplaza inspección visual.

---

# 11. Protocolo para agentes LLM

## 11.1 Lectura obligatoria

Antes de cambiar el proyecto:

1. AGENTS.md.
2. ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md.
3. MVP_LA_PRIMERA_SENAL.md.
4. DEVELOPMENT_STATE.md.
5. BACKLOG.md.
6. Documento específico del sistema.
7. ADR relevantes.

Si todavía existe una contradicción, detener la implementación y resolver la documentación primero.

## 11.2 Unidad de trabajo

Cada tarea debe declarar:

~~~yaml
phase: 2
capability: patch_lod_neighbor_rule
objective: "Impedir una diferencia de LOD mayor a uno"
inputs:
  - FPlanetPatchAddress
  - PlanetPatchManager
outputs:
  - neighbor_constraint
tests:
  - deterministic_split
  - adjacent_lod_delta
acceptance:
  - "Delta LOD <= 1 en todos los vecinos activos"
out_of_scope:
  - stitching_final
~~~

## 11.3 Ciclo autónomo

1. Leer estado y criterios.
2. Elegir la tarea desbloqueada más prioritaria.
3. Definir cambio mínimo verificable.
4. Crear o actualizar la prueba.
5. Implementar.
6. Compilar.
7. Ejecutar pruebas.
8. Abrir test level si corresponde.
9. Medir.
10. Corregir.
11. Actualizar documentación.
12. Crear commit atómico sólo si queda estable.

## 11.4 Límites

Un agente NO DEBE:

- Generar un lote completo antes de validar una muestra.
- Declarar terminado sin evidencia.
- Incorporar un asset desde HF_Incoming directamente.
- Cambiar esqueleto después de aprobar una familia sin ADR.
- Actualizar Blender, Unreal o el add-on durante una producción activa.
- Introducir un plugin pago o servicio runtime.
- Ocultar fallos con defaults silenciosos.
- Usar una superficie plana como fallback de producción.

## 11.5 Prompt de inicio

~~~text
Leé completamente AGENTS.md, Docs/ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md,
Docs/DEVELOPMENT_STATE.md, Docs/BACKLOG.md y los ADR aplicables.

Identificá la fase activa y su puerta de salida. No trabajes sobre fases futuras
si la puerta actual no pasó. ASTRAEON exige planetas esféricos grandes:
ningún sistema nuevo puede asumir Z global como arriba ni usar Landscape plano
como fundamento.

Separá generación de datos, materialización visual y estado mutable. Toda
aleatoriedad persistente debe derivar de seeds estables y versiones explícitas.

Si la tarea crea arte, usá Higgsfield únicamente para propuesta o generación
inicial dentro de HF_Incoming. Después validá y corregí en Blender, registrá
procedencia, exportá con el preset aprobado, importá en Unreal y probá el asset
en un test level.

Implementá la capacidad mínima completa, compilá, ejecutá pruebas, medí,
documentá evidencia y actualizá el estado. No declares una fase terminada por
apariencia ni por compilación aislada.
~~~

---

# 12. Riesgos y mitigaciones

| Riesgo | Señal temprana | Mitigación |
|---|---|---|
| Grietas entre patches | Luz visible o colisión rota | Coordenadas compartidas, delta LOD ≤ 1, skirts, tests de bordes |
| Stutter de generación | Picos al volar | Workers, cancelación, colas de commit y presupuesto por frame |
| Jitter a gran escala | Cámara o props tiemblan | Doubles persistentes y frames locales |
| Cámara radial incómoda | Roll o saltos de orientación | Transporte suave del frame y pruebas en polos |
| Colisión tardía | Jugador cae | Anillo preventivo y prioridad por velocidad |
| HF produce mallas inútiles | Limpieza mayor que creación | Brief más estricto, blockout primero, descarte temprano |
| Blender 5.2.1 deja de funcionar con HF | Add-on no carga | Matriz de versiones e instalación compatible paralela |
| Morphs no viajan correctamente por FBX | Shape key ausente | Spike de ida y vuelta antes de producción masiva |
| 8 GB de VRAM insuficientes | Pool saturado | 2K por defecto, streaming, instancing, proxy y profiling |
| Ciudad procedural imposible | Rutas cortadas | Generación por capas y validador de transitabilidad |
| Universo inmanejable | Simulación consume CPU sin jugar | LOD de simulación y generación diferida |
| Scope creep | Muchas ramas sin vertical slice | Puertas duras y un solo objetivo por iteración |

## 12.1 Recortes permitidos

Para proteger el proyecto se puede reducir:

- Densidad de contenido.
- Cantidad de biomas.
- Cantidad de cuerpos activos.
- Complejidad de ciudades.
- Cantidad de familias de criaturas.
- Distancia de detalle.
- Simulación individual.

No se puede eliminar:

- Esfericidad real.
- Gravedad radial.
- Determinismo.
- Streaming.
- Continuidad perceptiva objetivo.
- Ciencia como herramienta.

---

# 13. Checklist formal de transición

## Documentación

- [ ] Crear ADR de planetas esféricos.
- [ ] Actualizar AGENTS.md.
- [ ] Actualizar MVP_LA_PRIMERA_SENAL.md.
- [ ] Actualizar GAME_DESIGN_MASTER.md.
- [ ] Actualizar ASTRAEON_PLAN_DESARROLLO_MAPA.md.
- [ ] Reclasificar radios de GUIA_ARTE_PLANETAS_BLENDER.md como Lab.
- [ ] Crear PHASE_STATUS.md.

## Código

- [ ] Aislar BP_PlanetGravityHarness.
- [ ] Crear módulo Planet.
- [ ] Crear PlanetDefinition.
- [ ] Crear PlanetCoordinateSystem.
- [ ] Crear PlanetGravityComponent.
- [ ] Crear cube-sphere cerrada.
- [ ] Crear primer test automatizado de seed.

## Arte

- [ ] Crear plantilla Blender.
- [ ] Registrar Blender, Higgsfield y preset FBX.
- [ ] Crear HF_Incoming.
- [ ] Ejecutar calibración de 1 m.
- [ ] Validar static mesh.
- [ ] Validar skeletal mesh.
- [ ] Validar una Action.
- [ ] Validar un shape key como morph.

## Producción

- [ ] Medir baseline.
- [ ] Etiquetar build recuperable.
- [ ] Definir responsable de cada puerta.
- [ ] Bloquear generación masiva hasta aprobar pipeline.

---

# 14. Definición de terminado del juego objetivo v1

ASTRAEON v1 está terminado cuando:

- Existe un sistema estelar persistente.
- El planeta principal y otro cuerpo son esféricos, grandes y explorables.
- Se puede despegar, navegar, aterrizar y volver.
- No hay pantallas de carga perceptibles en ese recorrido.
- El bucle explorar, medir, comprender, actuar y registrar sostiene una campaña completa.
- Ítaca funciona como hogar, herramienta y nave.
- El protagonista posee arte y animación aprobados.
- Los biomas afectan supervivencia y recursos.
- Las criaturas poseen nicho, comportamiento y variantes coherentes.
- Existe un primer contacto significativo.
- Una ciudad expresa civilización e historia.
- El guardado conserva seed, versiones y deltas.
- La partida funciona offline.
- El rendimiento pasa en el hardware objetivo.
- La build se reproduce desde un checkout documentado.
- No existen crashes conocidos en el recorrido crítico.
- Procedencia y licencias están registradas.

La expansión galáctica puede continuar después. La versión no necesita simular todo lo imaginable para cumplir la promesa central.

---

# 15. Referencias técnicas verificadas

Consultadas el 9 de septiembre de 2026:

- [Higgsfield for Blender — funciones, versiones y Bridge](https://higgsfield.ai/plugins/blender)
- [Unreal Engine — Set Gravity Direction](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/Pawn/Components/CharacterMovement/SetGravityDirection?application_version=5.7)
- [Unreal Engine — Large World Coordinates](https://dev.epicgames.com/documentation/unreal-engine/large-world-coordinates-in-unreal-engine-5)
- [Unreal Engine — World Partition](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine)
- [Unreal Engine — ProceduralMeshComponent 5.7](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/ProceduralMeshComponent?application_version=5.7)
- [Unreal Engine — FBX Content Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-content-pipeline)
- [Unreal Engine — Skeletal Mesh FBX Pipeline](https://dev.epicgames.com/documentation/unreal-engine/importing-skeletal-meshes-using-fbx-in-unreal-engine)
- [Unreal Engine — Chaos Destruction](https://dev.epicgames.com/documentation/en-us/unreal-engine/chaos-destruction-in-unreal-engine)
- [Blender Manual — FBX](https://docs.blender.org/manual/en/5.0/addons/import_export/scene_fbx.html)

Las páginas oficiales pueden mostrar documentación de una versión posterior. La implementación debe contrastarse siempre con Unreal 5.7.4 y con el test de calibración local.

---

# 16. Regla final

> ASTRAEON no construirá primero un juego plano para convertirlo después en un universo esférico. Construirá primero el núcleo planetario mínimo que demuestre la promesa; luego reconstruirá sobre él La primera señal y crecerá por fases hasta el juego objetivo.
