# Terreno procedural coherente — reemplazo de A03

Estado: decisión de producción. La malla de cuenca de prueba queda retirada del runtime.

## Resultado esperado

Cada seed genera una región natural y continua: llanuras, colinas, formaciones y recursos
parecen parte del mismo relieve. Nunca puede enterrar Ítaca, aislar la salida, dejar un POI bajo
el suelo ni crear una ruta crítica imposible.

## Cómo se genera

1. La `ContentSeed` deriva una `TerrainSeed` explícita.
2. Ruido de baja frecuencia define macrorelieve; ruido medio agrega lomas; detalle sólo cambia
   material/rocas, nunca la colisión básica.
3. Antes de crear la malla se trazan zonas de garantía: plataforma de Ítaca, salida, recursos,
   señal y dos corredores.
4. El generador suaviza y rebaja el relieve dentro de esas zonas; luego valida pendiente,
   conectividad y altura libre.
5. Si falla una invariante, reintenta con una sub-seed determinista. Tras un máximo definido,
   usa una variante segura documentada. No publica una seed inválida.

## Invariantes obligatorias

- Ítaca: plataforma plana, sin geometría dentro del volumen de nave ni bajo el piso.
- Salida: suelo exterior a la misma cota de la plataforma y espacio libre.
- Rutas: ancho mínimo 3 m, pendiente máxima validada contra la cápsula y continuidad desde
  Ítaca a recursos críticos y señal.
- POIs: centro y radio de interacción sin intersección con la malla.
- Mapa: límite regional claro, sin cráter circular artificial ni paredes generadas por cortes.
- Determinismo: misma seed y versión producen las mismas alturas y zonas de garantía.

## Dónde

- Generador/sampler: `Source/Astraeon/Private/WorldGen/`.
- Datos de restricciones: `AstraeonWorldProfileTypes.h` y `AstraeonWorldProfiles.cpp`.
- Malla y colisión: `AstraeonTerrainSurfacePrototype`, renombrada al aprobarse como sistema
  runtime.
- Validación: `AstraeonTerrainSurfaceTests.cpp`, smoke crítico y recorrido manual.

## Orden

1. Recuperar el terreno estable actual y retirar la malla experimental del build jugable.
2. Implementar sampler procedural con zonas de garantía y pruebas de invariante.
3. Generar malla continua sólo desde un sampler válido.
4. Integrar alturas de Ítaca, marcadores, criaturas y obras con una única consulta.
5. Probar al menos cinco seeds en editor y paquete antes de solicitar prueba humana.
