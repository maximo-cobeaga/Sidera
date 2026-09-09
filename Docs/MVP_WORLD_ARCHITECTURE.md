# Arquitectura de mundo y mapas — MVP

Estado: **decisión de arquitectura del propietario, 2026-09-06**.

## Decisión vinculante

El MVP no genera sistemas solares, planetas, lunas, continentes ni geografía principal a
partir de una `WorldSeed`. Su estructura será:

```text
Sistema estelar fijo → cuerpos definidos → regiones diseñadas → POIs principales fijos
                                                        ↓
                           PCG local y seeds secundarias para variación controlada
```

La proceduralidad sigue siendo determinista y offline, pero sólo altera contenido secundario:
rocas, flora, recursos secundarios, fauna, clima, encuentros y decoración. Nunca puede mover
o bloquear rutas críticas, zonas de aterrizaje, landmarks, POIs principales ni objetivos de
narrativa.

## Alcance de producción

La siguiente entrega es **ENVIRONMENT MVP — REGION A**, antes de producir otros cuerpos:

- región diseñada de 500 × 500 m, un bioma y una ruta crítica legible;
- una zona de aterrizaje y una nave/refugio;
- un POI principal, dos o tres POIs secundarios y landmarks fijos;
- familias modulares propias de roca, acantilado, planta, escombro, cristal y arquitectura;
- material maestro compartido e instancias por bioma;
- población local determinista para rocas, flora, recursos secundarios y fauna;
- rendimiento, rutas y contenido secundario comprobados con seeds `100`, `200`, `300`, `400`
  y `500` sin alterar geografía, POIs ni caminos.

El objetivo futuro de un sistema con tres cuerpos explorables, estación, campo de asteroides
y anomalía guía la arquitectura. No se implementa en esta entrega: se desbloquea sólo después
de que Region A sea reutilizable y se apruebe su alcance narrativo.

## Datos y responsabilidades

`PlanetProfile` y `RegionProfile` son datos estructurados, no resultados de una seed.
Como mínimo deberán identificar cuerpo/región, ambiente, gravedad, atmósfera, bioma,
recursos, fauna, clima, landing zones y POIs principales. En el futuro, un generador podrá
crear esos perfiles desde una seed; los sistemas de mundo consumirán el perfil sin conocer su
origen.

| Capa | Controla |
| --- | --- |
| Diseño de mundo | regiones, rutas, landmarks, POIs principales y narrativa |
| Profile | identidad ambiental, recursos, fauna, clima y reglas de población |
| PCG / variación | densidad, ubicación válida, variantes y encuentros secundarios |
| Assets | geometría modular, materiales, pivotes, LOD y colisiones |

Los assets se organizan por tier: Hero (nave, criatura relevante, artefactos, landmarks),
Environment (rocas, estructuras y flora reutilizable) y Filler (detalle económico e
instanciable). Blender conserva autoridad sobre escala, topología, pivote, materiales,
sockets y exportación; cualquier salida asistida por IA debe pasar por ese control.

## Restricciones

- Sin planeta esférico transitable, generación de continentes, sistema solar generado ni
  transición física continua órbita-superficie durante este MVP.
- Espacio, navegación interplanetaria y nuevos cuerpos permanecen planificación posterior
  hasta que Region A cierre su vertical slice.
- Usar materiales maestros, instancias, trim sheets, instancing y geometría moderada; no
  materiales ni texturas exclusivos por cada roca.
- Conservar componentes reutilizables de RNG, ruido, sampling, perfiles, reglas ambientales
  y bibliotecas de assets. Eliminar sólo duplicación o experimentos sin consumidor.

## Criterio de éxito de Region A

La región conserva identidad, rutas, landmarks y POIs con todas las seeds de contenido;
la variación es visible pero no rompe interacción, navegación ni rendimiento. Su arquitectura
debe poder reutilizarse para una segunda región sin duplicar lógica.

Todo trabajo de mundo debe responder: “¿mejora directamente Region A o sólo permite generar
más mundos?”. Si sólo genera más mundos, queda diferido.
