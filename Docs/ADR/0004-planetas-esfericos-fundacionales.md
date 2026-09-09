# ADR 0004 — Planetas esféricos y gravedad radial como requisito fundacional

## Estado

Aceptada por directiva del propietario el 2026-09-09.

Supersede parcialmente a [ADR 0003](0003-fixed-region-world-architecture.md): conserva su
decisión sobre regiones diseñadas y anula su orden de prioridades. Ver §Relación con ADR 0003.

## Contexto

Hasta hoy los planetas esféricos transitables eran una expansión tardía, y `AGENTS.md` §5 los
prohibía explícitamente durante el MVP. El vertical slice se construyó sobre una región plana de
500 × 500 m con gravedad global fija en −Z.

`ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` establece que esa promesa —superficie esférica grande,
gravedad radial y continuidad entre suelo, atmósfera, órbita y espacio— es identidad central del
juego, no una expansión. Construir más sistemas de superficie sobre un plano significa
construirlos dos veces.

Tres hechos medidos el 2026-09-09 sostienen que la migración es viable ahora y no más tarde:

- **No hay sistema de gravedad que desmontar.** `GravityMS2` es un escalar que modula velocidad de
  marcha y salto (`AstraeonSuitComponent.cpp:142-158`). No existe ninguna llamada a dirección de
  gravedad en el proyecto.
- **El motor ya expone lo necesario.** UE 5.7.4 local: `SetGravityDirection`
  (`CharacterMovementComponent.h:1675`), `ProjectToGravityFloor`, `GetGravitySpaceZ`,
  `HasCustomGravity`, `GetGravityTransform`. Verificado en la instalación, no supuesto.
- **El contrato de superficie ya se escribió para esto.** `AAstraeonTerrainField::GetHeightCm` es
  una función estática y pura de `(seed, x, y)`, y su propio header declara que se diseñó para que
  la generación planetaria pudiera consultarla antes de que el terreno existiera.

Y uno que fija el coste: **~43 de las 58 pruebas no consultan geometría** y sobreviven intactas.
El proyecto no se reinicia.

## Decisión

La superficie jugable final pertenece matemáticamente a un cuerpo esférico. En consecuencia:

- Ningún sistema nuevo **puede** asumir que el eje Z global representa arriba.
- El terreno final **no puede** basarse en Unreal Landscape plano.
- El tamaño del planeta es un **dato**, nunca un `Scale` de Blueprint ni una malla estática única.
- El mundo cercano se materializa mediante **patches cargados por demanda** sobre una cube-sphere.
- Una superficie plana **no es un fallback de producción** aceptable.
- La identidad persistente de un objeto **no** es su `Transform` mundial: es `BodyId` + dirección
  superficial + altitud + coordenadas locales.
- La esfera escalada actual se aísla como banco de pruebas `BP_PlanetGravityHarness` y no
  evoluciona por subdivisión manual.

El orden de trabajo es el de `Docs/PLAN_TRANSICION_EJECUCION.md`. Una fase termina por evidencia
contra su puerta de salida, no por compilar.

## Consecuencias

- **Corte inmediato del mundo plano.** `L_AstraeonBootstrap` se retira como mapa de producción.
  No habrá build jugable nueva hasta la Fase 3. La etiqueta `pre-transicion-plana` conserva la
  última build jugable del mundo plano y es lo único demostrable mientras tanto.
- **Cuarentena de pruebas.** Unas 15 pruebas pierden el mundo sobre el que corren. Se marcan con
  razón nombrada y se listan en `PLAN_TRANSICION_EJECUCION.md` §3.1, con la fase en que vuelven.
  Que esa tabla quede vacía es la puerta de salida de la Fase 3. Esto es una excepción acotada y
  registrada a `AGENTS.md` §7 —"no desactivar una prueba para lograr una compilación verde"—, no
  su derogación: ninguna prueba se borra, ninguna se relaja, y cada una tiene fecha de vuelta.
- **El estado mutable deja de ser deuda diferible.** La puerta de la Fase 2 exige que ir y volver
  regenere el mismo patch, y hoy `BACKLOG` MV4 registra que las criaturas muertas reaparecen al
  rematerializar. Con patches que cargan y descargan, ese agujero pasa de molestia a bloqueo. El
  Bloque B de `ESTADO_Y_RUTA_MAPA.md` queda absorbido por la Fase 2.
- **Los sectores planos se cancelan.** El Bloque C de `ESTADO_Y_RUTA_MAPA.md` queda superado: lo
  reemplaza el quadtree de patches por cara.
- **Los Data Assets de perfiles se difieren a la Fase 4**, deliberadamente: sus estructuras cambian
  de forma durante las Fases 1 y 2, y convertirlas hoy sería convertirlas tres veces.
- **Los sistemas sin dependencia geométrica se conservan sin tocar:** seeds y determinismo, ciencia
  ambiental, escáner, bitácora, inventario, crafting, traje, narrativa, criaturas, herramientas de
  Blender y manifiestos de assets.
- **Se abstraen o reemplazan:** gravedad global fija, movimiento sobre Z global, coordenadas de
  región cartesianas, colocación de props por normal global, dependencias de Landscape, navegación
  plana y guardados que persistan sólo un `Transform` mundial.
- **Los radios de `GUIA_ARTE_PLANETAS_BLENDER.md` (150 m a 2 km) dejan de ser tiers objetivo** y
  pasan a ser pruebas de laboratorio. Los tiers de ingeniería son Lab 10 km, Target 500 km y
  Stress 2500 km.
- **El backend de malla queda sin decidir a propósito.** `ProceduralMeshComponent` es experimental
  en 5.7.4; el prototipo puede usarlo detrás de `IPlanetPatchMeshBackend`, y la elección de
  producción se registra por ADR propio después de medir tiempo de commit, memoria, colisión y
  estabilidad.

## Relación con ADR 0003

**Se conserva:** la estructura del mundo no deriva de una `WorldSeed`; el juego consume regiones
diseñadas mediante perfiles, con POIs, rutas y landing zones estables, y las seeds deterministas
se restringen a contenido secundario validado. Eso sigue siendo cierto — cambia la geometría sobre
la que se proyecta, no el principio.

**Se anula:** la prioridad de Region A por delante del planeta, el espacio navegable y la
transición órbita-superficie. Region A ahora se materializa *sobre* el núcleo planetario, y ese
núcleo se construye primero. El prototipo de terreno continuo deja de estar aislado: su
`BuildMeshData` pasa a ser el constructor de patch.

## Alternativas descartadas

- **Terminar el vertical slice plano y migrar después.** Es la ruta que este ADR reemplaza. Cada
  sistema de superficie añadido mientras tanto —biomas, streaming de sectores, ciudades— se
  construiría dos veces, y el segundo intento sería más caro que el primero.
- **Escalar la esfera actual hasta el tamaño objetivo.** Una malla estática escalada no da LOD,
  colisión por anillos ni precisión a 500 km. Es el banco de pruebas, no el terreno.
- **Planeta voxel completo.** Resuelve cuevas y destrucción, pero su coste de memoria, generación
  y colisión no cabe en el presupuesto de una RTX 4060 de 8 GB ni en una producción individual.
  Las cuevas se resuelven con niveles y mallas modulares conectadas sobre un heightfield radial.
- **Mantener los dos mundos vivos hasta la Fase 3.** Máxima seguridad, pero cada cambio de gameplay
  y cada asset se pagan dos veces. Descartada por el propietario a favor del corte inmediato.
