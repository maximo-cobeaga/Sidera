# Contrato inicial de generación modular

2026-09-05. Documento creado en esta entrega; no existía durante la auditoría.

Los catálogos y la selección de módulos son datos separados de gameplay. El primer generador produce un kit estático de Ítaca a partir de `Tools/Blender/configs/itaca_blockout.json`, versión 1. Registra seed 13579 como procedencia, pero el kit fijo no depende de ella: no hay falsa promesa de variantes por seed.

Próximas familias usarán IDs estables, versiones explícitas y streams locales derivados de EntitySeed, sin aleatoriedad global. Versionar tanto algoritmo como catálogo; comparar geometría/IDs/parámetros esenciales, no bytes FBX que incluyen metadatos. No alterar el generador C++ de partidas para cambiar un color de arte.

Interfaces: módulos humanos sobre grilla de 2 m, pivote de suelo y XY centrado. Sockets y skeletons de personaje/cuadrúpedo son propuestas descritas en `ART_ASSET_MASTER_PLAN.md`, todavía sin implementación. Antes de congelarlos: importar un rig, probar articulaciones y conexiones, verificar deformación con todos los módulos límite.

Reutilizar `common/scene.py`, `validators/mesh.py` y `exporters/fbx.py`. La validación actual certifica sólo blockouts estáticos sin texturas. Añadir perfiles de UV productivo, texturas, armature, skin, LOD y destructibles junto con la primera familia que los necesite; no marcar esos controles como aprobados por no aplicarse hoy.
