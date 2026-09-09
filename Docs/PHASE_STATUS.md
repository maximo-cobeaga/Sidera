# Estado de fases — ASTRAEON

Fase activa, puerta de salida y cuarentena de pruebas. Es el archivo que un agente lee para saber
**qué está permitido hacer hoy**. Se actualiza al abrir y al cerrar cada fase.

Fases definidas en `ASTRAEON_TRANSICION_AL_JUEGO_OBJETIVO.md` §6. Orden de trabajo y criterios
detallados en `PLAN_TRANSICION_EJECUCION.md` §4.

---

## Fase activa: **0 — Transición controlada**

Abierta el 2026-09-09. Documentos y spike técnico **en paralelo** (decisión del propietario).

### Puerta de salida

- [x] No queda documento rector que prohíba la esfera ni que mande Landscape plano.
- [x] El proyecto compila y **las 65 pruebas pasan** (58 previas + 7 nuevas del marco planetario).
- [x] Existe la etiqueta `pre-transicion-plana` y su build recuperable está documentada.
- [x] La cuarentena de §Cuarentena está poblada, con fase de retorno por prueba.
- [x] La calibración Blender → Unreal respeta escala y ejes (`evidencia/CALIBRACION_BLENDER_UNREAL.md`).
- [~] El personaje se sostiene de pie: **medido**, vuelta completa a la esfera con 0,0% de frames desalineados (`-AstraeonSmokePlanetWalk`). Falta sólo el juicio humano sobre **cómo se siente la cámara**,
      probado a mano.

### Rama documental

- [x] `Docs/ADR/0004-planetas-esfericos-fundacionales.md`.
- [x] `AGENTS.md` sincronizado: misión, fuentes de verdad, §4.1 restricciones planetarias,
      §5 límites (la prohibición de la esfera transitable era el bloqueo), §7 cuarentena,
      §8 presupuesto, §9 seeds en workers, §11 identidad planetaria, §15 cierre por fase.
- [x] `Docs/PHASE_STATUS.md` (este archivo).
- [x] `Docs/MVP_LA_PRIMERA_SENAL.md`: la región se materializa sobre el planeta esférico.
- [x] `Docs/GAME_DESIGN_MASTER.md`: coordenadas, esfera y gravedad se adelantan.
- [x] `Docs/ASTRAEON_PLAN_DESARROLLO_MAPA.md`: cube-sphere y patches pasan a las primeras fases.
- [x] `Docs/GUIA_ARTE_PLANETAS_BLENDER.md`: radios de 150 m a 2 km reclasificados como Lab.
- [x] `Docs/ESTADO_Y_RUTA_MAPA.md`: Bloque C superado, Bloque B absorbido por la Fase 2.

### Rama técnica

- [x] Etiqueta `pre-transicion-plana` sobre `503e3a1` y build recuperable documentada.
- [x] Módulo `Planet/Coordinates/` y `Planet/Gravity/` creados con su estructura.
- [x] Harness aislado como `AAstraeonPlanetGravityHarness` (C++, no BP: no existía esfera previa que renombrar y `AGENTS.md` §4 reserva los BP para presentación).
- [x] `UAstraeonPlanetGravityComponent` mínimo: `SetGravityDirection` + orientación de cápsula.
- [x] Mapa `TL_10_RadialGravity` (radio 200 m, 4 balizas, generado por script).
- [x] Baseline de rendimiento en `Docs/evidencia/BASELINE_RENDIMIENTO.md` (dos mapas, JSON reproducible).

### Calibración Blender

- [x] Preset `UE57_AST_V1` congelado en `Tools/Blender/presets/`, con comprobación automática de deriva. Versiones de Blender y Unreal registradas. *(Higgsfield: sin generación en esta fase; su versión se registra cuando vuelva a usarse.)*
- [x] `TL_00_AssetCalibration`: cubo de 1 m, mannequin de 1,83 m y tres ejes de largos distintos.
- [x] Static mesh, skeletal mesh, una Action y un shape key como morph: los cuatro validados con números.

### Fuera de alcance en esta fase

Biomas finales, nave pilotable, ciudades y generación masiva de assets.

---

## Cuarentena de pruebas

Abierta por el corte inmediato del mundo plano (ADR 0004). Reglas en `AGENTS.md` §7: no se borra
ninguna, ninguna se relaja, y **una fase no cierra con pruebas suyas todavía aquí**.

Estado: **poblada al abrir la Fase 1**. Mientras el mapa plano siga en el repositorio, estas
pruebas siguen verdes y la tabla es sólo el inventario de lo que va a entrar.

| Prueba | Vuelve en | Estado |
|---|---|---|
| `Astraeon.WorldGen.Terrain.Relief` | Fase 1 | verde |
| `Astraeon.WorldGen.Terrain.SurfaceContract` | Fase 1 | verde |
| `Astraeon.WorldGen.Terrain.Connectivity` | Fase 2 | verde |
| `Astraeon.WorldGen.Terrain.TraversalDetectsWalls` | Fase 2 | verde |
| `Astraeon.WorldGen.Itaca.DeckRestsOnTerrain` | Fase 3 | verde |
| `Astraeon.WorldGen.Itaca.MaterializerSpecs` | Fase 3 | verde |
| `Astraeon.WorldGen.Region.MaterializerSpecs` | Fase 3 | verde |
| `Astraeon.WorldGen.Region.MarkerApplySpec` | Fase 3 | verde |
| `Astraeon.WorldGen.Region.MarkerVisualIdentity` | Fase 3 | verde |
| `Astraeon.Exploration.Map.RevealCell` | Fase 3 | verde |
| `Astraeon.Exploration.Map.RadiusAndPersistence` | Fase 3 | verde |
| `Astraeon.Art.Region.PresentationPreservesGameplay` | Fase 3 | verde |
| `Astraeon.Art.Itaca.ExteriorAndStations` | Fase 3 | verde |
| `Astraeon.Narrative.Itaca.SurfaceDeploymentLogbook` | Fase 3 | verde |
| `Astraeon.Functional.CriticalPath.FullFlow` | Fase 3 | verde |

`Astraeon.WorldGen.Terrain.TraversalDetectsWalls` merece vigilancia propia: existe para **exigir
que el validador falle** cuando la región no se puede recorrer. Si al migrarla a patches hay que
debilitar lo que pide para que apruebe, el defecto está en la migración.

---

## Fases siguientes

Ninguna empieza antes de que pase la puerta de la anterior.

| Fase | Nombre | Puerta principal | Estado |
|---:|---|---|---|
| 0 | Transición controlada | ADR y documentos sincronizados; spike de gravedad radial en pie | **activa** |
| 1 | Núcleo planetario | Gravedad y cámara estables sobre una esfera cerrada | pendiente |
| 2 | Patches, LOD y precisión | Streaming sin grietas ni hitches; estado mutable persistente | pendiente |
| 3 | La primera señal esférica | Vertical slice completo; **cuarentena vacía** | pendiente |
| 4 | Planeta visual y biomas | Seis biomas y rendimiento aprobado | pendiente |
| 5 | Protagonista, Ítaca y vuelo | Ida y vuelta sin carga perceptible | pendiente |
| 6 | Sistema estelar | Persistencia entre cuerpos | pendiente |
| 7 | Mundo vivo | Ecología, clima y progresión estables | pendiente |
| 8 | Primer contacto | Ciudad coherente, transitable y narrativa | pendiente |
| 9 | Sociedad y legado | Consecuencias persistentes | pendiente |
| 10 | Expansión y lanzamiento | Build estable, medida y testeada | pendiente |

Al pasar la Fase 2 queda cumplido el **Producto A** (núcleo planetario); al pasar la Fase 3, el
**Producto B** (vertical slice esférico); la Fase 10 cierra el **Producto C** (juego objetivo v1).
