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

---

## Estado — 2026-09-08

Los cinco pasos del orden quedaron hechos. Lo que faltaba no era la superficie, que ya se
materializaba, sino **la garantía**: nada comprobaba que se pudiera recorrer.

| Paso | Estado | Dónde |
|---|---|---|
| 1. Retirar la malla experimental del build jugable | Hecho | La cuenca authored de prueba ya no sale del constructor de malla. Antes bastaba con que el contexto trajera el id de Region A para cambiar el terreno entero |
| 2. Sampler con zonas de garantía y pruebas de invariante | Hecho | `AAstraeonTerrainField::SampleHeightCm` + `Astraeon.WorldGen.Terrain.Connectivity` |
| 3. Malla continua sólo desde un sampler válido | Hecho | La seed que se materializa es la que pasó la validación, no la que pidió la partida |
| 4. Alturas de Ítaca, marcadores, criaturas y obras con una única consulta | Hecho | `UAstraeonGameInstance::GetSurfaceHeightCm` |
| 5. Cinco seeds en editor y paquete | Hecho | 7 seeds en la prueba (100–500, 13579, 1001) y smoke sobre el ejecutable |

### Cómo se valida el tránsito

`FAstraeonTerrainTraversal::Evaluate` inunda la superficie desde la salida de la escotilla,
**a la misma resolución con la que se construye la malla** (400 cm): entre dos vértices la
malla es un plano, así que validar a otra resolución mediría una superficie que el jugador
no pisa. Dos muestras vecinas se consideran conectadas si el desnivel no supera el escalón
de la cápsula (45 cm) o la pendiente caminable del `CharacterMovement` (44,765°), lo que sea
mayor: el límite es el del motor, no uno inventado, porque es el que decide de verdad si una
ladera se sube o se resbala.

Objetivos exigidos: los cinco recursos —los tres del recorrido crítico y las dos vetas
profundas—, la señal, la anomalía y los dos nidos. Un objetivo fuera del campo cuenta como
inalcanzable en vez de recortarse contra el borde de la rejilla.

### Qué se hace cuando una seed no pasa

`ResolveTerrainSeed` prueba hasta 8 sub-seeds deterministas derivadas de la pedida
(`DeriveSeed(seed, "TerrainRetry:N")`), y publica la primera que deja todo alcanzable. Si
ninguna sirviera, recurre a la variante segura documentada (13579), que la propia prueba
comprueba con los mismos objetivos, y lo deja en el log como error.

**Esto no era teoría:** con la seed por defecto **1001** la región dejaba un objetivo
inalcanzable, y el juego lo publicaba igual. Ahora el log de una partida normal dice:

```
AstraeonTerrainSeed: seed pedida 1001 descartada tras 3 intentos; se materializa -1297777899.
Inalcanzable con la pedida: minor_geologic_anomaly
```

### Dos defectos encontrados al validar

- **El campo de terreno seguía a la nave.** Estaba centrado en Ítaca, así que aterrizar
  movía el relieve entero bajo un contenido que está fijo en el mundo, y las esquinas de la
  región podían quedar fuera del campo. Ahora el campo se centra en la **región** y la nave
  sólo aporta su huella plana. Aterrizar se limita al interior de la región, que es donde
  hay terreno.
- **Las criaturas caminaban sobre la capa de suelo desnuda**, que ignora claros, exclusiones
  de montaña y la plataforma de Ítaca. Donde había montaña permitida se hundían metros bajo
  lo que se ve. Las cuatro consultas de altura del runtime pasan ahora por la misma función.
