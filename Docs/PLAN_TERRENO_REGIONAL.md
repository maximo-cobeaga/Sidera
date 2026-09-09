# Plan de desarrollo — terreno regional propio

Fecha: 2026-09-06. Responsable de seguimiento: dirección de desarrollo.
Estado: **reorientado por ADR 0003**. T00 permanece terminado. T01 queda diferido: la
superficie continua sólo se evaluará contra geografía diseñada de Region A, no por seed.

## 1. Objetivo y resultado esperado

Entregar una región acotada, continua y diseñada donde el jugador pueda salir de
Ítaca, elegir una ruta, medir, recolectar, evitar una criatura, alcanzar la señal y regresar.
El paisaje debe comunicar superficies caminables, barreras y referencias de orientación.

Presupuesto de compras: **cero**. Usar código propio, materiales propios, Blender y Unreal
ya instalados. Sin descargar assets ni introducir dependencias nuevas en este bloque.
Mantener Unreal 5.7 y la plataforma Windows 11 x64.

Prioridad: resolver geometría y recorrido antes de producir detalle visual. La propuesta
preferida es una malla continua; su viabilidad se decide mediante un prototipo medido.
Referencias de aceptación: AC-02, AC-03, AC-05, AC-06, AC-07, AC-11, AC-13 y AC-14 de
[MVP_LA_PRIMERA_SENAL.md](MVP_LA_PRIMERA_SENAL.md).

## 2. Punto de partida comprobado por lectura

- `AAstraeonTerrainField` materializa cubos instanciados de 700 cm de lado, con capas de
  suelo y montañas. Ya existen funciones de altura, exclusiones y plataforma de Ítaca.
- La altura sin modificar y la superficie final no son siempre equivalentes: intervienen
  claros, exclusiones de montañas, plataforma y muestreo de baldosas. El reemplazo debe
  resolver esta diferencia para todos los consumidores de altura.
- Hay material geológico propio y pipeline Blender/Unreal reutilizable. Las manos,
  herramientas, criatura y nave ya tienen arte Q1: no reconstruir esos lotes.
- Hay pruebas de relieve y de apoyo de Ítaca. El límite actual de escalón es 45 cm;
  una superficie continua necesita además verificar pendientes y conectividad.
- La criatura se mueve sin barrido de colisión. Su corrección se incluye como integración
  mínima para que paredes y refugio sigan teniendo sentido sobre el terreno nuevo.
- Existen vuelo local, obras y persistencia. Se prueban por regresión; este plan no amplía
  esos sistemas ni resuelve la discrepancia histórica entre documentos de fase.

La verificación de ejecución más reciente debe releerse al iniciar T00; los resultados
históricos no garantizan el estado del árbol de trabajo futuro.

## 3. Orden, responsabilidades y dependencias

Los responsables son roles de trabajo; una misma persona o agente puede cubrirlos.
No implica abrir sesiones paralelas. Mantener una sola tarea técnica activa hasta su salida.

| ID | Prioridad | Trabajo | Responsable | Depende de | Estado |
| --- | --- | --- | --- | --- | --- |
| T00 | P0 | Línea base y contrato de superficie | Ingeniería + QA | — | Terminado |
| A01 | P0 | Datos y layout fijo de Region A | Diseño + Ingeniería | T00 | Terminado |
| A02 | P0 | Integrar perfiles a sesión, materialización y saves | Ingeniería + QA | A01 | Terminado |
| A03 | P0 | Superficie diseñada de Region A | Diseño de nivel + Ingeniería | A02 | Pendiente |
| T01 | P0 | Prototipo de malla continua y decisión técnica | Ingeniería | A03 | Diferido: debe consumir superficie diseñada |
| T02 | P0 | Integración de alturas, Ítaca y persistencia | Ingeniería | T01 aceptado | Pendiente |
| T03 | P0 | Movimiento de criaturas con obstáculos | Gameplay + QA | T02 | Pendiente |
| T04 | P1 | Dos rutas y composición regional | Diseño de nivel | T02, T03 | Pendiente |
| T05 | P1 | Materiales y kit de rocas originales | Arte técnico | T04 | Pendiente |
| T06 | P0 de cierre | Regresión, rendimiento y build | QA + Ingeniería | T00–T05 | Pendiente |

Siguiente tarea al reanudar: **A03**, definir e implementar la superficie diseñada de Region A.
T01 se retoma sólo al tener esa superficie disponible para medirla.

### Evidencia T00 — 2026-09-06

- Consumidores inventariados: materialización y marcadores (`AstraeonGameModeBase`), spawn y
  apoyo de criatura, teletransporte/trace de escotilla, extracción/construcción, vuelo y
  diagnósticos. La base anterior `GetGroundHeightCm` no contiene plataforma, claros ni
  exclusiones de montaña, por lo que no es suficiente como altura final.
- Contrato añadido: `FAstraeonTerrainSurfaceContext` → `SampleSurface`. Entrada: seed efectiva,
  centro del campo, claros, zonas sin montaña y cota de Ítaca. Salida: altura en centímetros,
  normal unitaria y validez dentro de radio de 320 m. Es puro y no necesita actores.
- Línea base: baldosas de 700 cm, campo de radio 320 m, escalón máximo declarado 45 cm,
  cápsula de jugador radio 42 cm / semialtura 96 cm; caminata 520 cm/s y sprint 900 cm/s.
- Build `AstraeonEditor Win64 Development`: exitosa en 32,38 s. Automation: 53 éxitos,
  exit code 0; los tres `Condition failed` previos a la cola son el aviso conocido del motor.

### Evidencia T01 parcial — 2026-09-06, diferida por ADR 0003

- Habilitado únicamente el plugin oficial de Epic incluido en la instalación UE 5.7,
  `ProceduralMeshComponent`; ADR 0002 registra alcance y consecuencias.
- `AAstraeonTerrainSurfacePrototype` crea malla y colisión solicitada a partir del contrato,
  a 400 cm entre muestras: 25.921 vértices y 51.200 triángulos para 640 × 640 m.
- `Astraeon.WorldGen.Terrain.SurfaceContract` comprueba las diez seeds, determinismo,
  normal, plataforma, límites del campo y topología. Aprobó dentro de la suite de 53 tests.
- Falta antes de aceptarlo: que consuma geografía diseñada de Region A, spawn runtime, trazos
  y barridos contra su colisión, tiempo de cocción/memoria y comparación renderizada. Por ello
  T01 no desbloquea T02 aún.

### A01 — Definir los perfiles y el layout fijo de Region A

**Qué:** reemplazar la dependencia conceptual de `WorldSeed` como geografía por un perfil
estable de planeta y región, con landing zone, rutas, POI principal y POIs secundarios.

**Cómo:** crear estructuras C++ y configuración textual/versionable; elegir un único cuerpo
y una sola Region A de 500 × 500 m. Fijar coordenadas, ambiente, biome, límites de población,
recursos, fauna y objetivos narrativos. Mantener `EnvironmentSeed`, `ResourceSeed`,
`FaunaSeed`, `WeatherSeed` y `EncounterSeed` sólo para variación secundaria. Diseñar y probar
una ruta directa expuesta y una alternativa segura sobre la geografía definida. No crear aún
otros planetas, espacio o World Partition.

**Dónde:** tipos en `Source/Astraeon/Public/WorldGen/`; datos textuales en `Config/` o
`Source/Astraeon/Private/WorldGen/`; esquema en `Docs/DISENO_RECORRIDO_REGIONAL.md`; contrato
en `Docs/MVP_WORLD_ARCHITECTURE.md`. La decisión de formato exacto se registra antes de migrar
guardados o `GameInstance`.

**Esperado:** Region A se carga por `RegionProfileId`, conserva siempre geografía y POIs
principales, y acepta cinco seeds de variación sin alterar caminos u objetivos. Pruebas de
datos validan IDs, coordenadas, referencias, rangos ambientales y separación entre contenido
fijo/secundario. A01 desbloquea T01 y adaptación de materialización.

**Resultado, 2026-09-06:** completado como contrato de datos y diseño, sin cambiar la sesión
vigente. `planet_khepri` y `region_first_signal_basin` viven en
`UAstraeonWorldProfiles`; su ambiente, cinco recursos, señal, anomalía y dos nidos son fijos.
`Astraeon.WorldGen.Profiles.RegionA` valida el perfil. Recorrido, zonas reservadas y rutas en
`Docs/DISENO_RECORRIDO_REGIONAL.md`. A02 integra el perfil; A03 define geografía antes de T01.

### A02 — Integrar perfiles, sesión y persistencia

**Resultado, 2026-09-06:** completado. `StartNewGame` usa el PlanetProfile y el layout fijo de
Khepri/Region A; `ContentSeed` conserva la variación elegida sin poder alterar recursos, POIs
ni ambiente authored. El HUD identifica variación y región. SaveGame v2 persiste `PlanetProfileId`,
`RegionProfileId` y `ContentSeed`. Al cargar v1 se preserva la región previa, se la marca como
legado y el siguiente snapshot la migra a v2. La prueba `Astraeon.Persistence.SaveGame.V1Migration`
cubre la compatibilidad. Mientras A03 no entregue geografía authored, el campo temporal se fija
a `EnvironmentSeed=100` para que la semilla de contenido no mueva el suelo bajo rutas ni POIs.

## 4. Fichas de ejecución: qué, cómo, dónde y qué se espera

### T00 — Establecer línea base y contrato de superficie

**Qué:** registrar el comportamiento actual y definir la consulta de superficie que usará
la región completa.

**Cómo:** preservar cambios existentes; capturar la misma seed a altura de cámara y desde
vuelo; medir inicio, generación y aterrizaje. Inventariar llamadas de altura y trazos de
colisión. Definir entrada (seed, versión, coordenadas y contexto regional), salida (altura,
normal y validez) y límites del campo. Documentar cm en la interfaz Unreal y conversiones.
La consulta debe describir la superficie triangulada efectiva, incluyendo claros y
plataforma, y poder calcularse antes de crear los actores.

**Dónde:** `Source/Astraeon/Public/WorldGen/AstraeonTerrainField.h`, su `.cpp` en
`Private/WorldGen/`, `AstraeonRegionMaterializer.cpp`, `AstraeonGameModeBase.cpp` y
`Docs/TEST_REPORT.md`. Diseño del contrato en `Docs/DECISIONS.md`.

**Esperado:** lista completa de consumidores, capturas comparables y métricas iniciales.
Matriz fija de diez seeds: `11, 22, 42, 123, 999, 4242, 13579, 24680, 65535, 104729`.
Verificar valores de pendiente/cápsula efectivos del Character; no asumirlos por documentación.

### T01 — Probar una superficie continua

**Qué:** prototipo aislado que elimine escalones de baldosas en una región de prueba.

**Cómo:** muestrear el campo determinista sobre una grilla, triangular, calcular normales
y generar colisión coherente. Elegir resolución mediante comparación medida; evitar una
malla excesiva por defecto. Si se divide en secciones, compartir posiciones y normales en
bordes. Revisar primero capacidades de malla runtime disponibles en la instalación y su
empaquetado; no asumir que un componente o módulo está habilitado. Comparar coste de
generación, colisión y memoria con T00. Mantener el terreno previo como alternativa de
prueba hasta aceptar el prototipo, sin destruir mapas existentes.

**Dónde:** implementación en `Private/WorldGen/` y contratos en `Public/WorldGen/`;
pruebas nuevas propuestas en `Private/Tests/AstraeonTerrainSurfaceTests.cpp`.
Registrar la decisión en un ADR nuevo con número libre dentro de `Docs/ADR/`.

**Esperado:** misma seed reproduce geometría esencial; ausencia de grietas y triángulos
inválidos; barridos y trazos coinciden con superficie visual. La adopción requiere prototipo
compilado, probado y medido. Si necesita una dependencia nueva, documentar la limitación y
evaluar alternativas disponibles antes de modificar el entorno.

### T02 — Integrar la superficie con el mundo y el guardado

**Qué:** colocar correctamente estancia, jugador, marcadores, criaturas y obras existentes.

**Cómo:** centralizar consultas sobre la superficie final. Mantener plataforma bajo Ítaca,
grosor de cubierta y acceso a escotilla sin doble suelo ni paredes de roca. Comprobar
transformaciones al iniciar, aterrizar y reconstruir la región. Los barridos deben validar
espacio libre, no solamente altura de un punto. Conservar ids de recursos, recetas y saves.
Antes de cambiar el generador, decidir compatibilidad: conservar reconstrucción antigua o
migrar con prueba explícita. No mover silenciosamente obras ni jugadores de saves previos.

**Dónde:** `AstraeonGameModeBase.cpp`, `WorldGen/AstraeonRegionMaterializer.cpp`,
`Environment/AstraeonItacaInterior.cpp`, `Ship/AstraeonShipPawn.cpp`,
`AstraeonPlayerCharacter.cpp`, `Creatures/AstraeonCreatureActor.cpp`, todos en
`Source/Astraeon/Private/`; persistencia en `AstraeonGameInstance.cpp` y
`Public/Persistence/AstraeonSaveGame.h`. Revisar `Private/Building/` como consumidor.

**Esperado:** tres recursos y señal accesibles, criatura apoyada, escotilla transitable,
sin caídas ni rescates causados por terreno. Save anterior de prueba y save nuevo conservan
progreso, inventario, mapa y deltas después de cerrar/abrir. Ampliar
`Private/Tests/AstraeonItacaGroundTests.cpp` y pruebas de persistencia existentes.

### T03 — Corregir movimiento de criaturas contra geometría

**Qué:** impedir atravesar paredes y relieve manteniendo amenaza evitable.

**Cómo:** reproducir cruce de pared; introducir movimiento con comprobación de colisión y
respuesta mínima al bloqueo (detenerse o desviar/recalcular destino). Probar antes de adoptar
una solución de navegación mayor. Ajustar apoyo al terreno sin teletransportar a través de
obstáculos. Conservar prohibición de subir sobre el animal y revisar daño a través de paredes.

**Dónde:** `Private/Creatures/AstraeonCreatureActor.cpp`, cabecera correspondiente y
`Private/Tests/`; evidencias y reproducción en `Docs/KNOWN_ISSUES.md`.

**Esperado:** patrulla y persecución sobre pendiente; pared bloquea movimiento y no permite
daño por contacto a través de ella; animal no arrastra la cámara ni queda bloqueado
permanentemente en el recorrido de prueba. Escaneo y retirada siguen funcionando (AC-06).

### T04 — Diseñar dos rutas y referencias visuales

**Qué:** convertir el campo caminable en un recorrido legible con una decisión de navegación.

**Cómo:** establecer una ruta directa más expuesta y otra de rodeo para evitar la criatura,
usando amenazas existentes. Crear esquema en planta, hitos y vistas a altura del jugador.
Definir corredores por anchura de cápsula, pendiente transitable y obstáculos reales;
comprobar conectividad sobre la superficie con colisión, no sólo entre centros de grilla.
Reservar accesos a nodos y escotilla. Distribuir referencias por seed sin bloquear objetivos.

**Dónde:** esquema y reglas propuestas en `Docs/DISENO_RECORRIDO_REGIONAL.md`;
`Private/WorldGen/AstraeonWorldGenerator.cpp`, `AstraeonRegionMaterializer.cpp` y datos
en `Public/WorldGen/AstraeonRegionTypes.h`, sólo donde sean necesarios.

**Esperado:** ambas rutas llegan al objetivo en las diez seeds; se puede reconocer regreso
a Ítaca por referencias visuales y completar la expedición sin vuelo, trucos o teletransporte
de rescate. Registrar duración y puntos de desorientación; el objetivo global de 30–45 minutos
se evalúa sobre la sesión completa y no se fuerza alargando artificialmente los caminos.

### T05 — Crear materiales y rocas del proyecto

**Qué:** dos superficies base (regolito y roca) y tres siluetas de roca reutilizables,
con variantes paramétricas discretas.

**Cómo:** reutilizar el material geológico y el pipeline original; variar por pendiente,
escala y seed. Modelar en Blender con configuración textual, pivotes al suelo, normales
correctas, escala documentada y colisión sencilla. Mantener paleta compatible con recursos.
Distribuir sólo después de reservar corredores. Ajustar luz/bruma existente para lectura;
cielo físico nuevo queda fuera de este entregable. Validar desde cámara y vista aérea.

**Dónde:** `Scripts/Editor/PrepareItacaTerrainMaterials.py` como referencia existente;
generador/config propuestos en `Tools/Blender/generators/` y `Tools/Blender/configs/`;
importador en `Scripts/Editor/`; paquetes propuestos en
`Content/Astraeon/Art/Blockouts/Terrain/`; manifiesto/reportes en `ContentPipeline/`.
Verificar convención real de carpetas antes de crear las nuevas fuentes.

**Esperado:** fuentes reproducibles, importación comprobada, rocas apoyadas y pasos libres.
Materiales sin patrón de cuadrícula evidente en capturas de referencia y recursos legibles
a distancia de interacción. Calidad de prototipo coherente, sin exigir arte final.

### T06 — Validar y entregar la build del bloque

**Qué:** demostrar estabilidad y capacidad jugable con el conjunto integrado.

**Cómo:** compilar `AstraeonEditor` y `Astraeon` Win64 Development; ejecutar Automation
`Astraeon.*`, smoke de mapa y recorrido crítico tanto editor-game como empaquetado.
Extender scripts existentes `Scripts/RunItacaChecks.ps1` y `RunRegionArtChecks.ps1`
cuando corresponda; registrar comandos exactos realmente ejecutados.

**Dónde:** pruebas en `Source/Astraeon/Private/Tests/`; logs en `Saved/Logs/`;
build propuesta en `Builds/WindowsTerrainRegional/`; evidencia resumida en
`Docs/TEST_REPORT.md`, `DEVELOPMENT_STATE.md`, `BACKLOG.md` y `KNOWN_ISSUES.md`.

**Esperado:** diez seeds verificadas automáticamente; tres recorridos completos consecutivos
sin crash en build renderizada; guardar/cerrar/continuar; tres ciclos de vuelo/aterrizaje
como regresión de lo ya existente. El smoke que teletransporta entre objetivos debe
complementarse con recorrido a pie: por sí solo no demuestra transitabilidad.

Medir en 1920×1080, registrando hardware, ajustes, FPS promedio, percentiles de tiempo de
frame, pico de generación/aterrizaje, inicio y memoria durante una sesión de 30–45 minutos.
Referencia: i5-14400F, RTX 4060 8 GB, 16–32 GB RAM; objetivo 60 FPS, mínimo provisional
45 FPS sin stutter persistente; inicio menor a 45 s en SSD. Si el hardware difiere,
declararlo y no certificar cumplimiento en el equipo de referencia. Comparar con T00.

## 5. Riesgos y decisiones de gestión

| Riesgo | Tratamiento y condición de avance |
| --- | --- |
| Colisión de malla demasiado costosa | Medir en T01; ajustar resolución/secciones antes de integrar. |
| Altura matemática distinta de la colisión | Consulta de superficie final compartida y pruebas contra trazos reales. |
| Save previo queda bajo el suelo | Versionado/migración explícita con fixtures; bloquear adopción si falla. |
| Rocas vuelven a bloquear Ítaca | Exclusiones por volumen y barridos de accesos, después de decoración. |
| Apariencia bonita pero recorrido imposible | Conectividad y caminata real obligatorias antes del cierre. |
| Cambios ajenos sin consolidar | Separar diffs propios; commits atómicos sólo de trabajo estable. |
| Documentación histórica contradictoria | Usar este plan para este bloque y registrar evidencia vigente al ejecutar T00. |

No incluye nuevas especies, planetas completos, civilizaciones, sistemas de construcción,
UMG, VFX, audio, ni rediseño de nave. Esos temas conservan su planificación separada.

## 6. Seguimiento y cierre

Actualizar cada ID con estado, responsable efectivo, archivos cambiados, comandos,
resultado y deuda restante. Estados: Pendiente → En curso → En verificación → Terminado;
usar Bloqueado sólo con causa y evidencia documentadas. No marcar terminado por existir
una malla o por aprobar únicamente pruebas de fórmulas.

El bloque se cierra cuando T00–T06 tienen evidencia, existe build reproducible, el recorrido
es completo y los problemas materiales están resueltos o explícitamente pendientes de
aceptación. Entregar controles y limitaciones y solicitar una sesión humana del conjunto.
Esto no declara cerrado el MVP completo ni autoriza su expansión.

**Entrega actual:** únicamente este plan y su registro en documentos vivos. Todas las
tareas de implementación permanecen pendientes por instrucción del propietario.
