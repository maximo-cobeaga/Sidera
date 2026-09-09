# ASTRAEON: Plan técnico de desarrollo del mapa dinámico

**Proyecto:** ASTRAEON: Legado del Vacío  
**Motor objetivo:** Unreal Engine 5.7.4  
**Plataforma inicial:** Windows 11 x64  
**Modo:** aventura científica, primera persona, principalmente offline y para un jugador  
**Documento:** planificación técnica del mapa dinámico, ciudades, civilizaciones y sistema planetario abierto

---

## 1. Propósito del documento

Este documento define el orden lógico para construir el sistema de mapas de Astraeon. El objetivo final es disponer de un universo persistente compuesto por múltiples planetas, regiones, ciudades, civilizaciones e historias generadas de forma determinista.

El mapa debe cumplir cuatro propiedades fundamentales:

1. Una misma seed debe producir el mismo universo.
2. Las ciudades deben reflejar la civilización que las fundó.
3. La historia debe modificar el estado visible y jugable del mundo.
4. El jugador debe poder desplazarse progresivamente entre regiones, planetas y espacio sin percibir pantallas de carga.

La visión completa se construirá por etapas. El primer objetivo sigue siendo el vertical slice **La primera señal**, que debe validar la base técnica antes de incorporar civilizaciones avanzadas y múltiples planetas.

---

## 2. Principios técnicos obligatorios

### 2.1 Separación de datos y lógica

Los datos del mundo no deben estar codificados directamente en actores o Blueprints individuales. Deben estar separados de la lógica mediante estructuras configurables.

Se deben distinguir:

- Datos de planetas.
- Datos de biomas.
- Datos de recursos.
- Datos de criaturas.
- Datos de civilizaciones.
- Datos de edificios.
- Datos de ciudades.
- Datos de eventos históricos.
- Datos de misiones.
- Estado mutable de la partida.

La lógica debe leer esos datos y generar el mundo. No debe existir una ciudad cuyo comportamiento dependa de valores escondidos dentro de un Blueprint específico.

### 2.2 Determinismo

Todas las decisiones generadas deben depender de una seed explícita y de un identificador estable.

No se debe utilizar una única secuencia global de números aleatorios para todo el universo, porque modificar un objeto secundario podría cambiar la ubicación de una ciudad o un evento importante.

La seed principal debe derivar seeds independientes:

```text
MainSeed
├── GalaxySeed
├── PlanetSeed(PlanetId)
├── RegionSeed(PlanetId, RegionId)
├── BiomeSeed(PlanetId, RegionId, BiomeId)
├── CivilizationSeed(PlanetId, CivilizationId)
├── CitySeed(PlanetId, CivilizationId, CityId)
├── ResourceSeed(PlanetId, RegionId, ResourceLayer)
├── CreatureSeed(PlanetId, RegionId, SpeciesId)
└── EventSeed(PlanetId, EraId, EventIndex)
```

El sistema debe usar una función de hash estable para derivar cada seed. No se debe depender del orden de ejecución de los actores ni de direcciones de memoria.

### 2.3 Generación por capas

El mundo debe generarse en capas con dependencias claras:

```text
Seed
  ↓
Sistema estelar
  ↓
Planeta
  ↓
Regiones y biomas
  ↓
Civilizaciones
  ↓
Historia y relaciones
  ↓
Ciudades y asentamientos
  ↓
Edificios, rutas, NPCs y recursos
  ↓
Eventos jugables y estado persistente
```

Una capa no debe intentar resolver responsabilidades de otra. Por ejemplo, el generador de edificios no debe decidir la historia de una civilización.

### 2.4 Mundo estático y estado mutable

Debe distinguirse entre:

- **Mundo base:** lo que se obtiene nuevamente a partir de la seed.
- **Estado de partida:** cambios provocados por el jugador o por la simulación.

Ejemplos de mundo base:

- Posición de una ciudad.
- Tipo de arquitectura.
- Bioma.
- Recursos iniciales.
- Civilización fundadora.

Ejemplos de estado mutable:

- Ciudad destruida.
- Puente reparado.
- Facción que controla una región.
- NPC muerto o trasladado.
- Recurso agotado.
- Evento ya descubierto.

El guardado debe registrar solamente la seed y las diferencias necesarias, no una copia completa de cada objeto visual.

---

## 3. Arquitectura general del sistema

La arquitectura lógica recomendada es:

```text
AstraeonWorldSubsystem
├── SeedManager
├── GalaxyGenerator
├── PlanetGenerator
├── RegionGenerator
├── BiomeGenerator
├── CivilizationGenerator
├── HistorySimulation
├── SettlementGenerator
├── CityLayoutGenerator
├── AssetAssemblySystem
├── WorldStreamingManager
├── RuntimeStateManager
├── SaveGameManager
└── DebugWorldInspector
```

### Responsabilidades

| Sistema | Responsabilidad |
|---|---|
| `SeedManager` | Crear y derivar seeds estables |
| `GalaxyGenerator` | Crear sistemas, órbitas y relaciones entre cuerpos |
| `PlanetGenerator` | Crear parámetros globales del planeta |
| `RegionGenerator` | Dividir el planeta en regiones jugables |
| `BiomeGenerator` | Determinar clima, relieve, flora y recursos |
| `CivilizationGenerator` | Crear civilizaciones y sus características |
| `HistorySimulation` | Crear eventos, relaciones y consecuencias |
| `SettlementGenerator` | Elegir tipo, ubicación y función de asentamientos |
| `CityLayoutGenerator` | Crear calles, distritos y edificios |
| `AssetAssemblySystem` | Montar assets modulares según reglas |
| `WorldStreamingManager` | Cargar y descargar sectores cercanos |
| `RuntimeStateManager` | Aplicar cambios producidos durante la partida |
| `SaveGameManager` | Persistir seed y estado mutable |
| `DebugWorldInspector` | Mostrar seeds, IDs, capas y errores |

---

# 4. Fase 0: Contratos de diseño y base técnica

## Objetivo

Definir las reglas y los formatos de datos antes de crear contenido masivo. Esta fase evita que los sistemas posteriores queden atados a decisiones improvisadas.

## Subfase 0.1: Alcance y límites

Documentar claramente tres niveles:

### MVP: La primera señal

Debe incluir:

- Una región planetaria.
- Generación determinista por seed.
- Variables científicas ambientales.
- Exploración en primera persona.
- Escáner.
- Recursos y crafting.
- Una criatura principal.
- Bitácora.
- Mapa.
- Ruina o señal.
- Guardado y carga.

No debe depender de:

- Galaxia completa.
- Múltiples civilizaciones.
- Genética.
- Guerras.
- Colonias.
- Planetas esféricos completos.
- Viaje interestelar.
- Arte final completo.

### Demo tecnológica posterior

Debe validar:

- Una segunda región.
- Una ciudad modular.
- Una civilización básica.
- Historia local.
- Streaming entre regiones.

### Visión completa

Debe contemplar:

- Múltiples planetas.
- Ciudades dinámicas.
- Civilizaciones completas.
- Relaciones diplomáticas.
- Economía y migración.
- Historia persistente.
- Viaje superficie-espacio.
- Viaje entre planetas sin pantallas de carga perceptibles.

## Subfase 0.2: Convenciones del proyecto

Definir y aplicar:

- Prefijos de clases.
- Nombres de assets.
- Nombres de Data Assets.
- Estructura de carpetas.
- Unidades de medida.
- Ejes y orientación.
- Escala del jugador.
- Escala de edificios.
- Escala de regiones.
- Convenciones de IDs.

Propuesta de carpetas:

```text
Source/Astraeon/
├── Core/
├── World/
│   ├── Seed/
│   ├── Galaxy/
│   ├── Planets/
│   ├── Regions/
│   ├── Biomes/
│   └── Streaming/
├── Civilization/
├── History/
├── Settlements/
├── Assets/
├── Gameplay/
├── Save/
└── Debug/

Content/Astraeon/
├── Data/
├── World/
├── Biomes/
├── Civilizations/
├── Cities/
├── Buildings/
├── Creatures/
├── Materials/
├── Maps/
└── Test/
```

## Subfase 0.3: Contratos de datos

Crear estructuras equivalentes a:

```text
WorldDefinition
├── MainSeed
├── GalaxyId
├── GenerationVersion
└── Planets[]

PlanetDefinition
├── PlanetId
├── PlanetSeed
├── Radius
├── Gravity
├── Atmosphere
├── TemperatureRange
├── WaterCoverage
├── BiomeDistribution
└── Regions[]

CivilizationDefinition
├── CivilizationId
├── OriginRegionId
├── BiologyProfile
├── TechnologyLevel
├── GovernmentType
├── EconomyType
├── Values
├── ArchitectureStyle
├── Materials
├── Colors
├── Symbols
└── Relations[]

CityDefinition
├── CityId
├── CivilizationId
├── RegionId
├── SettlementType
├── Population
├── FoundedAt
├── Districts[]
├── InfrastructureState
├── HistoricEvents[]
└── RuntimeState
```

## Criterio de terminado

- Los datos pueden crearse sin modificar la lógica.
- Existe una seed principal y seeds derivadas.
- Los IDs son estables.
- El proyecto compila.
- Existe una prueba que genera dos veces el mismo resultado y compara los datos.
- El alcance del MVP está documentado y no se amplía accidentalmente.

---

# 5. Fase 1: Región planetaria jugable

## Objetivo

Construir una región pequeña pero completa que permita recorrer, medir, explorar, escanear, recolectar, fabricar y descubrir la primera señal.

## Subfase 1.1: Terreno base

Implementar:

- Terreno jugable.
- Zona de inicio.
- Límites de la región.
- Elevaciones.
- Pendientes transitables y no transitables.
- Caminos naturales.
- Área de interés.
- Ruina o estructura de señal.

El terreno debe admitir una variante generada por seed sin romper el recorrido principal. La ruta crítica debe garantizarse mediante reglas de validación, no mediante azar puro.

## Subfase 1.2: Regiones lógicas

Dividir la región en celdas o sectores lógicos. Cada sector debe tener:

- `RegionId`.
- Coordenadas lógicas.
- Bounds.
- Bioma.
- Nivel de detalle.
- Recursos permitidos.
- Puntos de interés.
- Estado de generación.

Las celdas lógicas no necesitan coincidir necesariamente con los tiles visuales. Su propósito es controlar generación, streaming y persistencia.

## Subfase 1.3: Reglas de transitabilidad

Antes de colocar objetos, validar:

- Que el punto inicial sea accesible.
- Que la señal sea accesible.
- Que los recursos mínimos estén alcanzables.
- Que no existan pendientes imposibles en la ruta crítica.
- Que el jugador pueda regresar o completar la secuencia prevista.

Se debe incluir una prueba automática de navegación o, como mínimo, una prueba de conectividad sobre la malla lógica de la región.

## Criterio de terminado

- El jugador completa el recorrido principal en 30–45 minutos.
- La región se genera con una seed.
- Una misma seed mantiene la topología y los puntos críticos.
- El mapa muestra la región y sus puntos descubiertos.
- La partida puede guardarse y cargarse correctamente.

---

# 6. Fase 2: Generación determinista de biomas y recursos

## Objetivo

Agregar variación controlada sin afectar la jugabilidad principal.

## Subfase 2.1: Perfil de bioma

Cada bioma debe definir:

- Temperatura.
- Humedad.
- Altitud preferida.
- Tipo de suelo.
- Color de terreno.
- Vegetación válida.
- Rocas válidas.
- Recursos posibles.
- Criaturas posibles.
- Condiciones ambientales.
- Densidad de objetos.

Ejemplo:

```text
BiomeProfile
├── ClimateRange
├── ElevationRange
├── GroundMaterial
├── VegetationSet
├── RockSet
├── ResourceSet
├── CreatureSet
├── DensityRules
└── ExclusionRules
```

## Subfase 2.2: Capas de generación

Generar en orden:

1. Relieve.
2. Agua y drenaje.
3. Biomas.
4. Rutas naturales.
5. Recursos principales.
6. Rocas y vegetación.
7. Detalles secundarios.
8. Puntos de interés.

Los objetos importantes deben generarse antes que los objetos decorativos. Las reglas decorativas nunca deben tapar entradas, rutas críticas, zonas de interacción o puntos de escaneo.

## Subfase 2.3: Distribución de recursos

Los recursos deben depender de:

- Bioma.
- Geología.
- Altitud.
- Humedad.
- Distancia a puntos de interés.
- Rareza definida.
- Seed de recursos.

Debe existir una cantidad mínima garantizada de recursos para completar el objetivo del MVP. La generación puede agregar variación, pero nunca dejar al jugador sin una solución válida.

## Criterio de terminado

- La misma seed produce los mismos recursos y objetos importantes.
- Diferentes seeds producen variaciones visibles.
- La ruta principal siempre es completables.
- El rendimiento se mantiene dentro del presupuesto definido.
- Se pueden regenerar sectores sin duplicar objetos.

---

# 7. Fase 3: Sistema de civilizaciones

## Objetivo

Crear civilizaciones como entidades de datos con características que puedan influir en la arquitectura, economía, comportamiento e historia.

## Subfase 3.1: Perfil de civilización

Cada civilización debe tener:

- Identidad.
- Origen.
- Biología o forma de vida.
- Bioma natal.
- Recursos disponibles.
- Nivel tecnológico.
- Organización política.
- Economía.
- Valores.
- Religión o cosmovisión, si corresponde.
- Arquitectura.
- Materiales.
- Colores.
- Símbolos.
- Vestimenta.
- Tipos de herramientas.
- Relaciones con otras civilizaciones.
- Objetivos actuales.

## Subfase 3.2: Reglas de coherencia

El generador debe validar contradicciones.

Ejemplos:

- Una civilización desértica no debe depender de una economía basada en agricultura abundante sin una fuente de agua definida.
- Una civilización subterránea no debe generar ciudades con grandes estructuras abiertas sin justificar su función.
- Una civilización con tecnología limitada no debe utilizar elementos tecnológicos incompatibles.
- Una civilización aislada no debe comenzar con relaciones comerciales extensas sin una ruta válida.

Las reglas deben producir advertencias o rechazar configuraciones imposibles.

## Subfase 3.3: Relaciones entre civilizaciones

Representar relaciones como datos:

```text
CivilizationRelation
├── CivilizationA
├── CivilizationB
├── RelationType
├── TrustValue
├── TradeValue
├── ConflictValue
├── SharedHistory[]
└── CurrentStatus
```

Tipos iniciales:

- Neutral.
- Comercial.
- Aliada.
- Rival.
- Hostil.
- Dependiente.
- Desconocida.

## Criterio de terminado

- Se pueden crear perfiles diferentes sin cambiar código.
- Cada perfil modifica al menos arquitectura, materiales y economía.
- Las relaciones se guardan mediante IDs estables.
- El validador detecta combinaciones incoherentes.

---

# 8. Fase 4: Historia emergente y simulación del mundo

## Objetivo

Crear una historia por partida que tenga consecuencias visibles y jugables.

## Subfase 4.1: Línea temporal

Definir una línea temporal por eras:

```text
Era 0: formación del planeta
Era 1: aparición de la vida
Era 2: surgimiento de civilizaciones
Era 3: expansión tecnológica
Era 4: conflictos y migraciones
Era 5: situación al comenzar la partida
```

La historia generada no debe ser texto aislado. Debe ser una secuencia de eventos con actores, causas, resultados y consecuencias.

## Subfase 4.2: Eventos históricos

Cada evento debe tener:

- ID.
- Fecha o era.
- Tipo.
- Actores.
- Causa.
- Resultado.
- Territorio afectado.
- Recursos afectados.
- Relaciones modificadas.
- Evidencias presentes en el mundo.
- Estado histórico.

Ejemplo:

```text
HistoryEvent
├── EventId
├── Era
├── Type
├── Actors[]
├── Cause
├── Outcome
├── AffectedRegions[]
├── RelationChanges[]
├── WorldEvidence[]
└── Consequences[]
```

## Subfase 4.3: Traducción de historia a mundo

Un evento debe convertirse en elementos jugables:

| Evento | Consecuencia visible |
|---|---|
| Guerra | Murallas, ruinas, puestos militares |
| Migración | Barrio abandonado, nueva arquitectura |
| Descubrimiento | Laboratorio, mina o zona de investigación |
| Catástrofe | Área contaminada, clima alterado |
| Cambio político | Nuevos símbolos, guardias y leyes |
| Crisis económica | Mercados vacíos, escasez y rutas modificadas |

## Subfase 4.4: Simulación limitada

La simulación debe empezar con variables simples:

- Población.
- Recursos.
- Seguridad.
- Producción.
- Comercio.
- Confianza.
- Control territorial.
- Estabilidad.

Cada ciclo de simulación puede actualizar las ciudades y civilizaciones. No se debe simular cada ciudadano individualmente en esta etapa.

## Criterio de terminado

- Dos seeds pueden generar historias diferentes.
- La historia está representada mediante datos.
- Los eventos generan evidencias en el escenario.
- Las relaciones cambian por eventos.
- El jugador puede descubrir parte de la historia explorando.

---

# 9. Fase 5: Generación de asentamientos y ciudades

## Objetivo

Crear ciudades que respondan a la civilización, el bioma, la geografía y la historia local.

## Subfase 5.1: Selección del asentamiento

El sistema debe decidir:

- Tipo de asentamiento.
- Ubicación.
- Tamaño.
- Población.
- Función principal.
- Civilización fundadora.
- Recursos que motivaron su creación.
- Nivel de defensa.
- Conexiones con otras ciudades.

Tipos iniciales:

- Aldea minera.
- Ciudad comercial.
- Puerto.
- Fortaleza.
- Colonia científica.
- Capital política.
- Refugio subterráneo.
- Estación orbital.
- Ciudad abandonada.

La ubicación debe validarse contra pendiente, agua, recursos, rutas y peligros.

## Subfase 5.2: Distritos

Una ciudad debe estar formada por distritos funcionales, no por edificios colocados al azar.

Distritos posibles:

- Centro político.
- Residencial.
- Industrial.
- Comercial.
- Religioso.
- Científico.
- Militar.
- Agrícola.
- Portuario.
- Logístico.

La selección de distritos debe depender de la economía, gobierno, tecnología, bioma e historia.

## Subfase 5.3: Gramática urbana

Definir reglas de conexión:

```text
Centro
├── Plaza o núcleo principal
├── Rutas hacia entradas
├── Distrito político
├── Distrito comercial
├── Distrito residencial
└── Distritos especializados
```

El generador debe construir primero:

1. Límites y topografía.
2. Entradas y salidas.
3. Rutas principales.
4. Centro urbano.
5. Distritos.
6. Edificios principales.
7. Viviendas.
8. Decoración y detalles.

## Subfase 5.4: Coherencia arquitectónica

Cada edificio debe declarar:

- Estilo arquitectónico compatible.
- Materiales permitidos.
- Tamaño.
- Función.
- Requisitos tecnológicos.
- Requisitos de distrito.
- Variantes disponibles.
- Puntos de conexión.
- Puntos de entrada.
- Colisión.
- LODs.

El sistema no debe mezclar módulos incompatibles solamente porque encajan geométricamente.

## Subfase 5.5: Historia física de la ciudad

El estado histórico debe modificar la ciudad:

- Ciudad recién fundada: construcción incompleta.
- Ciudad próspera: expansión y mantenimiento alto.
- Ciudad en guerra: defensas y daños.
- Ciudad en decadencia: edificios cerrados y rutas deterioradas.
- Ciudad abandonada: vegetación, ruinas y saqueo.
- Ciudad ocupada: símbolos y guardias diferentes.

## Criterio de terminado

- Una ciudad se genera a partir de `CitySeed`.
- La ciudad tiene rutas transitables.
- La ciudad pertenece visual y funcionalmente a una civilización.
- Los distritos tienen propósito.
- El estado histórico cambia su aspecto y composición.
- Dos seeds producen ciudades diferentes pero válidas.

---

# 10. Fase 6: Assets modulares e integración visual

## Objetivo

Crear un conjunto limitado pero reutilizable de piezas que permita generar variedad sin producir miles de assets únicos.

## Subfase 6.1: Familias modulares

Priorizar:

- Piezas de terreno.
- Caminos.
- Muros.
- Puertas.
- Ventanas.
- Techos.
- Soportes.
- Pisos.
- Escaleras.
- Módulos de interiores.
- Decoración.
- Recursos.
- Vegetación.
- Señalética.

Cada familia debe tener variantes compatibles y reglas de conexión.

## Subfase 6.2: Contrato de asset

Cada asset debe incluir:

- Nombre estable.
- Escala correcta.
- Pivote definido.
- Ejes correctos.
- Colisión.
- Materiales asignados.
- LODs.
- Bounds correctos.
- Tags funcionales.
- Categoría.
- Requisitos de bioma o civilización.
- Presupuesto de polígonos.

## Subfase 6.3: Ensamblaje

El sistema de ensamblaje debe seleccionar assets usando:

- Tipo de edificio.
- Civilización.
- Bioma.
- Nivel tecnológico.
- Estado histórico.
- Seed de variante.

Debe ser posible reemplazar un asset sin modificar el generador de ciudades.

## Criterio de terminado

- Los assets pasan validación de escala, pivote, materiales y colisión.
- Se pueden ensamblar edificios sin intervención manual.
- Las variantes respetan el estilo de cada civilización.
- Los assets tienen LODs y presupuesto de rendimiento.
- Una falta de asset produce un fallback controlado, no un bloqueo del juego.

---

# 11. Fase 7: Streaming de regiones y mundo abierto

## Objetivo

Permitir que el jugador recorra un mundo grande cargando solamente lo necesario.

## Subfase 7.1: Sectores de streaming

Cada sector debe tener:

- ID estable.
- Bounds.
- Dependencias.
- Nivel de detalle.
- Estado de generación.
- Estado de carga.
- Objetos persistentes.

Estados recomendados:

```text
Unloaded → Requested → Generating → Loading → Active
Active → Unloading → Unloaded
```

## Subfase 7.2: Distancias de carga

Definir anillos alrededor del jugador:

- Anillo cercano: geometría completa, colisión y gameplay.
- Anillo medio: geometría simplificada y gameplay limitado.
- Anillo lejano: representación visual simplificada.
- Fuera de rango: sólo datos de simulación.

## Subfase 7.3: Persistencia

Al descargar un sector se deben conservar:

- Objetos modificados.
- Recursos recolectados.
- Puertas abiertas o cerradas.
- NPCs importantes.
- Daños.
- Eventos descubiertos.
- Cambios de control territorial.

El sector debe poder regenerarse desde la seed y luego aplicar el estado guardado.

## Criterio de terminado

- El jugador puede moverse entre varios sectores.
- Los sectores se cargan y descargan sin duplicación.
- El jugador no cae fuera del mundo durante la transición.
- Los cambios persisten después de descargar y volver a cargar.
- El rendimiento es estable durante el desplazamiento.

---

# 12. Fase 8: Sistema planetario abierto

## Objetivo

Extender la arquitectura desde una región planetaria a múltiples planetas y sectores espaciales.

## Subfase 8.1: Catálogo planetario

Cada planeta debe registrar:

- `PlanetId`.
- Nombre.
- Seed.
- Tipo.
- Radio.
- Gravedad.
- Atmósfera.
- Temperatura.
- Cobertura de agua.
- Biomas.
- Civilizaciones presentes.
- Regiones disponibles.
- Estado global.

## Subfase 8.2: Representaciones por distancia

El mismo planeta debe tener varias representaciones:

1. Datos abstractos, cuando está muy lejos.
2. Representación orbital, al acercarse.
3. Superficie simplificada, durante la aproximación.
4. Región jugable, al aterrizar.

No se debe intentar renderizar con máximo detalle todos los planetas al mismo tiempo.

## Subfase 8.3: Identidad de coordenadas

Implementar una jerarquía de coordenadas:

```text
Coordenada galáctica
  → Coordenada del sistema
    → Coordenada orbital
      → Coordenada planetaria
        → Coordenada regional
          → Coordenada local
```

Las conversiones deben ser deterministas y centralizadas. Ningún actor debe calcular por su cuenta la posición planetaria.

## Criterio de terminado

- Existen varios planetas persistentes.
- Cada planeta puede tener regiones y seeds distintas.
- El jugador puede seleccionar y alcanzar otro planeta.
- El estado de un planeta se conserva al abandonar y regresar.
- El sistema no necesita mantener todos los detalles cargados.

---

# 13. Fase 9: Viaje continuo superficie-espacio

## Objetivo

Conectar superficie, atmósfera, órbita y espacio mediante transiciones continuas o visualmente ocultas.

## Subfase 9.1: Estados de la nave

Definir estados explícitos:

```text
Docked
→ Launching
→ AtmosphericFlight
→ Orbit
→ InterplanetaryTravel
→ PlanetApproach
→ AtmosphericEntry
→ Landing
→ Docked
```

Cada estado debe controlar:

- Movimiento.
- Cámara.
- Input.
- Representación del planeta.
- Streaming.
- Simulación.
- Condiciones de transición.

## Subfase 9.2: Transición espacial

Durante un viaje:

- El planeta de origen pasa a representación simplificada.
- Se prepara el sistema de destino.
- Se cargan datos del planeta, no todo su contenido visual.
- Se genera o activa la región de llegada.
- Se precargan puntos cercanos al aterrizaje.

## Subfase 9.3: Precisión y escalas

La distancia entre planetas y la escala del jugador son incompatibles en un único espacio numérico sin una estrategia específica. Por eso deben separarse:

- Simulación orbital.
- Navegación espacial.
- Representación visual.
- Coordenadas locales de gameplay.

La nave puede viajar mediante una simulación espacial abstracta y cambiar progresivamente a una representación detallada al acercarse al destino, siempre que la transición sea coherente para el jugador.

## Criterio de terminado

- El jugador puede despegar, viajar y aterrizar.
- No se pierde la seed ni el estado del planeta.
- No aparecen objetos duplicados al volver.
- La transición no rompe cámara, input, física ni guardado.
- El sistema mantiene un rendimiento aceptable en el equipo objetivo.

---

# 14. Fase 10: Validación, rendimiento y calidad

## Objetivo

Garantizar que el contenido generado sea válido, reproducible y jugable.

## Subfase 10.1: Pruebas de determinismo

Comparar entre ejecuciones:

- IDs de planetas.
- IDs de regiones.
- Posiciones de ciudades.
- Biomas.
- Recursos.
- Relaciones.
- Eventos históricos.
- Composición de edificios.

La comparación debe generar un informe legible.

## Subfase 10.2: Pruebas de coherencia

Validar:

- Que una ciudad tenga una civilización válida.
- Que todos los edificios sean compatibles.
- Que los distritos tengan conexiones.
- Que las rutas sean transitables.
- Que existan recursos mínimos.
- Que los eventos tengan actores válidos.
- Que no existan IDs duplicados.
- Que no haya referencias rotas.

## Subfase 10.3: Presupuesto de rendimiento

Medir:

- Tiempo de generación.
- Tiempo de carga de sector.
- Memoria.
- Cantidad de actores.
- Triángulos visibles.
- Draw calls.
- Uso de CPU.
- Uso de GPU.
- Tamaño del guardado.
- Stutters durante streaming.

## Subfase 10.4: Herramientas de diagnóstico

Crear un modo de depuración que permita visualizar:

- Seed actual.
- IDs de planeta, región, ciudad y distrito.
- Bioma.
- Facción propietaria.
- Rutas.
- Bounds de streaming.
- Estado de generación.
- Dependencias.
- Objetos faltantes.
- Razón por la que una regla rechazó una posición.

## Criterio de terminado

- Las pruebas se ejecutan de forma repetible.
- Los fallos generan mensajes accionables.
- No se ocultan errores mediante valores por defecto silenciosos.
- El mundo puede regenerarse y compararse automáticamente.
- El rendimiento se mide antes de agregar más contenido.

---

# 15. Orden de implementación recomendado

El orden correcto es:

```text
1. Contratos de datos y seeds
2. Región jugable del MVP
3. Guardado y carga
4. Biomas y recursos
5. Streaming de sectores
6. Primera ciudad modular
7. Primera civilización
8. Historia local
9. Varias ciudades
10. Varias regiones
11. Catálogo de planetas
12. Viaje espacial
13. Transición superficie-espacio
14. Escala, optimización y contenido adicional
```

No se debe avanzar de fase solamente porque el código compile. Cada fase requiere pasar sus pruebas funcionales, deterministas y de rendimiento.

---

# 16. Qué debe producir cada iteración

Cada iteración de desarrollo debe entregar:

1. Un objetivo único.
2. Cambios acotados.
3. Código y datos.
4. Una prueba automatizada o reproducible.
5. Una escena de validación.
6. Un informe de resultado.
7. Actualización del estado de desarrollo.
8. Registro de decisiones técnicas.
9. Problemas conocidos.
10. Próximo objetivo.

Una iteración no debe intentar resolver simultáneamente planeta, ciudad, civilización, nave y narrativa.

---

# 17. Definición de terminado del primer gran hito

El primer gran hito se considera terminado cuando existe una región planetaria que:

- Se genera a partir de una seed.
- Tiene un recorrido jugable.
- Contiene biomas y recursos.
- Tiene una criatura escaneable.
- Incluye una ruina o señal.
- Permite medir variables ambientales.
- Permite fabricar una solución.
- Registra descubrimientos en una bitácora.
- Se puede guardar y cargar.
- Se puede regenerar idénticamente.
- No depende de múltiples planetas ni civilizaciones avanzadas.

El segundo gran hito será una ciudad perteneciente a una civilización, con una historia local que cambie su arquitectura, sus rutas, sus recursos y sus puntos de interés.

---

# 18. Riesgos principales

## Alcance excesivo

Intentar crear todo el universo antes de validar una región provocaría sistemas incompletos y difíciles de depurar.

**Mitigación:** mantener el MVP como una región completa y funcional.

## Proceduralidad sin control

El azar puro puede producir ciudades imposibles o recorridos sin solución.

**Mitigación:** usar reglas, validadores, rutas críticas garantizadas y generación por capas.

## Assets incompatibles

Los módulos visuales pueden no respetar escala, pivotes, colisiones o estilos.

**Mitigación:** contrato obligatorio de assets y validación automática antes de integrarlos.

## Streaming inestable

La carga dinámica puede causar duplicados, pérdida de estado o tirones.

**Mitigación:** probar primero streaming de sectores pequeños y registrar explícitamente el estado persistente.

## Historia desconectada del gameplay

Una historia generada sólo como texto no aporta valor jugable.

**Mitigación:** cada evento debe producir evidencias, cambios de estado o consecuencias explorables.

## Escala planetaria

Un sistema continuo de superficie a espacio introduce problemas de precisión, streaming y rendimiento.

**Mitigación:** separar coordenadas, simulación y representación visual; implementar esta capacidad después del sistema regional.

---

# 19. Regla final de arquitectura

La regla más importante del sistema es:

> La seed genera el mundo base; las reglas generan la civilización; la historia modifica el estado; la ciudad expresa visualmente todo lo anterior; el guardado conserva únicamente los cambios.

Si una nueva función no puede explicar de dónde obtiene sus datos, qué seed utiliza, qué sistema es responsable de generarla y cómo se guarda su estado, todavía no está lista para incorporarse al proyecto.

