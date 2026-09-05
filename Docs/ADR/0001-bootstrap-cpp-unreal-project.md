# ADR 0001 — Bootstrap C++ textual del proyecto Unreal

## Estado

Aceptada

## Contexto

El repositorio inicial sólo contenía documentación. El MVP exige núcleo de gameplay en C++, pruebas y un proyecto Unreal Engine 5.7 para Windows 11.

## Decisión

Crear manualmente una base C++ mínima:

- `Astraeon.uproject`
- targets `Astraeon` y `AstraeonEditor`
- módulo runtime `Astraeon`
- GameMode y Character C++
- generador ambiental determinista
- bitácora y SaveGame mínimos
- Automation Tests iniciales

## Consecuencias

- Permite compilar y verificar lógica antes de producir assets binarios.
- Mantiene Blueprints como capa futura de composición visual.
- Requiere una tarea posterior para generar el primer `.umap` y assets de entrada si se decide usar Enhanced Input con acciones/assets.
