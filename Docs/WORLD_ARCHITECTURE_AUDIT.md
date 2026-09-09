# Auditoría de arquitectura de mundo — 2026-09-06

Fuente de decisión: [MVP_WORLD_ARCHITECTURE.md](MVP_WORLD_ARCHITECTURE.md).
Esta revisión clasifica el código actual; no elimina ni modifica comportamiento de gameplay.

| Sistema actual | Estado | Acción y motivo |
| --- | --- | --- |
| `FAstraeonEnvironmentalSnapshot`, fórmulas y clasificación de riesgo | KEEP | Ciencia y traje consumen datos estructurados; mover su origen de seed a profile. |
| RNG determinista y funciones de ruido | KEEP | Útiles para contenido secundario, PCG local y variación. |
| `UAstraeonWorldGenerator::IsBreathable` / riesgo ambiental | KEEP | Reglas independientes de cómo se creó el mundo. |
| `UAstraeonWorldGenerator::GenerateEnvironment` | ADAPT | Sustituir parámetros aleatorios por `PlanetProfile`/`RegionProfile`; conservar validadores. |
| `UAstraeonWorldGenerator::GenerateRegionLayout` | ADAPT | POIs críticos y rutas pasarán a región diseñada; la seed sólo poblará contenido secundario válido. |
| `FAstraeonRegionLayout::WorldSeed` y `GeneratorVersion` | ADAPT | Añadir identificadores estables de planeta/región y seeds por subsistema; migrar guardados con prueba. |
| `UAstraeonGameInstance::StartNewGame` y selector de seed | ADAPT | Nueva partida seleccionará Region A fija; conservar seed opcional para variación secundaria/QA. |
| `AAstraeonGameModeBase::MaterializeCurrentRegion` | ADAPT | Materializar datos de RegionProfile y niveles diseñados; no reubicar POIs principales por seed. |
| `AAstraeonRegionMaterializer` | ADAPT | Mantener especificaciones y spawn; consumir POIs fijos y resultados PCG validados. |
| `AAstraeonTerrainField` de cubos | DEFER | Geografía procedural por seed no es el terreno MVP. Reutilizar sampling/materiales sólo si Region A lo necesita. |
| `AAstraeonTerrainSurfacePrototype` y `SampleSurface` | DEFER | Conservados como experimento técnico; deben muestrear geografía diseñada antes de integrarse. |
| `ProceduralMeshComponent` oficial | KEEP | Capacidad de render/collision reutilizable; no implica adoptar geografía generada. |
| Marcadores de recursos, señal y anomalía | ADAPT | IDs, interacción y persistencia se conservan; posiciones de misión pasan a fijas. |
| Spawn/respawn de criaturas | ADAPT | Perfil y reglas útiles; nidos principales pasan a datos de región y secundarios a población local. |
| Construcción y persistencia de obras | KEEP | No asumen el origen procedural de la región; validar contra la futura superficie diseñada. |
| PlanetGenerator, SeedManager, PCG, BiomeProfile, PlanetProfile, World Partition | DEFER | No existen en el repositorio; crear sólo en el orden de Region A, sin implementar planetas completos. |
| Sistema solar, otros planetas/luna, estación, asteroides, transición espacial | DEFER | Arquitectura futura; fuera de Region A actual. |

## Orden de ejecución actualizado

1. Definir `PlanetProfile` y `RegionProfile` textuales/C++ para una Region A fija.
2. Diseñar datos de rutas, landing zone, POI principal y POIs secundarios de Region A.
3. Adaptado: la sesión y el guardado ya usan perfiles sin romper el recorrido actual; saves v1
   se conservan como legado y se migran al regrabar.
4. Terminar la superficie diseñada y sus colisiones; sólo entonces decidir si reutilizar el
   prototipo continuo o un Landscape diseñado.
5. Producir familias modulares y población local determinista.
6. Validar cinco seeds secundarias y reutilizar la arquitectura en Region B.

No hay elementos REMOVE por ahora: el código actual es pequeño y cada parte tiene ruta de
reutilización o se mantiene aislada como experimento.
