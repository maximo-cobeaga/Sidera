# ADR 0006 — Backend de malla de patches: ProceduralMeshComponent hasta la Fase 4

## Estado

Aceptada el 2026-09-10 al cerrar la Fase 2, con la autorización del propietario para cerrarla sin
consulta adicional. Cumple lo que pedía `PLAN_TRANSICION_EJECUCION.md` §4: *"la decisión de
producción se registra por ADR después de medir"*.

## Contexto

La Fase 2 construyó el streaming de patches detrás de una interfaz,
`IAstraeonPlanetPatchMeshBackend`: subir oculto, mostrar u ocultar, retirar. El gestor, el
selector, los workers y la colisión no conocen el backend. El prototipo es
`FAstraeonPlanetProceduralPatchBackend`, un `UProceduralMeshComponent` por patch con reutilización
de componentes.

Alternativas conocidas: Dynamic Mesh Component (Geometry Scripting), un componente propio con su
`FPrimitiveSceneProxy` y buffers en GPU, o mallas estáticas generadas y Nanite. Todas cuestan
más integración, y ninguna tenía una medición que las justificara.

## Medición

Unreal Insights y smokes de la Fase 2 (`TEST_REPORT.md`, P2.3-C y P2.4):

| Medida | Valor |
|---|---|
| Subida de un patch de 33×33 con faldones (`Astraeon_PlanetPatches_Commit`) | 0,08 ms de media, 0,5 ms de máximo |
| Construcción en worker (`Astraeon_PlanetPatches_Build`) | ~1,7 ms, fuera del hilo de juego |
| Componentes simultáneos en el vuelo de `TL_12` | 397 como máximo (tope de diseño 1.024) |
| Cocinado de colisión de un patch (2.048 triángulos) | 1,84 ms, 0,2 veces por segundo al caminar |
| Frame del vuelo de `TL_12` / caminata Target | p99 5,19 ms / 5,71 ms, con presupuesto de 16,67 |

El costo de dibujo y subida no aparece entre los temporizadores que importan. Lo que sí apareció,
el selector cuadrático, era lógica propia y ya se corrigió.

## Decisión

`ProceduralMeshComponent` es el backend de producción de las Fases 2 y 3. La interfaz se mantiene:
cambiarlo es sustituir una clase.

## Consecuencias

- La Fase 3 construye contenido sobre los patches sin cambiar de backend.
- **Se revisa en la Fase 4**, cuando lleguen materiales de bioma, más densidad de malla o
  sombreado por vértice: son los casos en los que un backend propio o Nanite pueden ganar. Antes de
  cambiar, se mide contra estas cifras.
- La colisión sigue cocinándose de forma síncrona. Pasar el anillo a cocinado asíncrono es una
  mejora posible y localizada (`KNOWN_ISSUES`, P2.4).

## Alternativa descartada por ahora

Un backend propio con buffers en GPU. Se descartó porque no hay medición que lo pida y su
integración —proxy, materiales, colisión aparte— es de un tamaño que la Fase 2 no necesitaba.
