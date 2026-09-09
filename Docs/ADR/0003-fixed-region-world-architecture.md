# ADR 0003 — Mundo fijo, regiones diseñadas y variación local

## Estado

Aceptada por directiva del propietario para el MVP.

## Decisión

La estructura del mundo no deriva de una `WorldSeed`: el MVP consume cuerpos y regiones
diseñados mediante `PlanetProfile`/`RegionProfile`. POIs narrativos, rutas, landmarks y
landing zones son estables. Las seeds deterministas se restringen a contenido secundario
validado dentro de esas regiones.

## Consecuencias

- La prioridad pasa a Region A, vertical slice ambiental de 500 × 500 m, antes de crear
  nuevos planetas, espacio navegable o transición órbita-superficie.
- Los generadores actuales se adaptarán en lugar de eliminarse; cualquier migración de save
  tendrá versión y prueba automática.
- El prototipo de terreno continuo queda aislado hasta que consuma geografía diseñada.
- Sistema solar, planetas/luna adicionales, estación y campo de asteroides son destino de
  arquitectura, no alcance de la siguiente implementación.
