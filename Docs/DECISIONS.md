# Decisiones — ASTRAEON

## 2026-09-05 — Normalizar documentos bajo `Docs/`

**Decisión:** copiar los documentos maestros existentes de la raíz a `Docs/` y mantener los originales por compatibilidad inicial.

**Motivo:** `AGENTS.md`, `README.md` y el prompt inicial declaran `Docs/` como ruta contractual, pero el repositorio inicial contenía los archivos en la raíz.

**Consecuencia:** las fuentes de verdad quedan disponibles en la ruta esperada sin eliminar archivos previos ni arriesgar pérdida de contexto.

## 2026-09-05 — Bootstrap manual de proyecto C++

**Decisión:** crear la estructura mínima de Unreal C++ directamente (`.uproject`, `Source/`, `Config/`) en lugar de depender de un template gráfico.

**Motivo:** es la alternativa más simple, verificable y controlada para iniciar H0 sin introducir contenido binario innecesario.

**Consecuencia:** falta generar un mapa `.umap` en una tarea posterior, pero el núcleo C++ puede compilarse y testearse temprano.
