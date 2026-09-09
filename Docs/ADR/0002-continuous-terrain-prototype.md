# ADR 0002 — Prototipo aislado de superficie continua

## Estado

Aceptada para T01; no adoptada todavía como terreno del recorrido principal.

## Contexto

El terreno actual usa cubos instanciados de 7 m. Aun con un desnivel limitado, sus caras
verticales se leen como capas y condicionan la navegación. Las funciones de altura base no
incluyen siempre claros, exclusiones de montaña ni plataforma de Ítaca, por lo que no pueden
ser el contrato de una malla continua sin corrección.

## Decisión

Habilitar el componente procedural oficial incluido con Unreal Engine 5.7 y usarlo sólo en
`AAstraeonTerrainSurfacePrototype`. No se descarga ni añade software externo. El actor crea
una malla de prueba de 4 m entre muestras para un campo de 640 m, con colisión asíncrona.

`FAstraeonTerrainSurfaceContext` y `SampleSurface` definen la consulta pura del terreno
final: seed efectiva, centro, claros, exclusiones de montañas y plataforma de Ítaca entran;
altura en cm, normal y validez salen. La malla y sus normales se derivan exclusivamente de
esa consulta.

## Consecuencias

- El terreno de cubos sigue siendo la implementación de gameplay; T02 sólo podrá sustituirlo
  tras pruebas de integración, guardado y rendimiento.
- El proyecto añade dependencia de compilación al plugin oficial de Epic
  `ProceduralMeshComponent`, incluido con la instalación fija UE 5.7. No cambia versión del
  motor, toolchain ni licencias.
- La prueba de malla valida topología/determinismo; aún falta medir cocción de colisión,
  trazos reales, memoria y ejecución renderizada antes de adoptar la alternativa.
