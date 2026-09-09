# Diseño de recorrido — Region A: Cuenca de la Primera Señal

Estado: layout diseñado para A01, 2026-09-06. Geografía final, PCG y arte siguen pendientes.

## Identidad

| Campo | Valor |
| --- | --- |
| Planeta | Khepri (`planet_khepri`) |
| Región | Cuenca de la Primera Señal (`region_first_signal_basin`) |
| Tamaño | 500 × 500 m; origen en zona de aterrizaje |
| Bioma | Desierto rocoso de baja gravedad y atmósfera no respirable |
| Refugio | Ítaca en `(0, 0)` m |
| Objetivo principal | Fuente de señal en `(220, 145)` m |
| Landmark principal | Cresta de señal al noreste, visible desde Ítaca |

La región debe leerse como una cuenca transitable: la cresta noreste atrae al jugador hacia
la señal, el borde sur ofrece rodeo protegido y la anomalía oeste abre una excursión opcional.
No se amplía el mapa con vacío: todas las distancias se justifican por descubrimiento, recurso,
peligro o referencia espacial.

## Layout fijo

```text
                         Norte

      anomalía                         nido este
      (-165,95)                            (130,120)
           \                                  /
            \                            señal (220,145)
             \                              /
              \       cresta directa       /
               \            ↑             /
Ítaca (0,0) --- ferrita (145,70) ---------
     \
      \ fibra (70,-35) -- cristal (190,-85) -- corredor sur
       \
        nido oeste (-75,-100)          vetas opcionales
```

Las posiciones canónicas viven en `UAstraeonWorldProfiles::GetRegionAProfile`; este dibujo
comunica intención, no sustituye datos de gameplay.

## Contrato de superficie A03

Los ejes de ambas rutas se almacenan como waypoints authored en el `RegionProfile`, en metros:
la directa `(0,0) → (70,-35) → (145,70) → (185,105) → (220,145)` y la segura
`(0,0) → (70,-35) → (190,-85) → (205,20) → (220,145)`. Cada eje reserva 3 m útiles;
Ítaca reserva 30 m y la señal 15 m. La superficie diseñada debe mantener esos corredores con
pendiente caminable y sin obstáculos de PCG. Estos datos pasan a ser la entrada obligatoria de
la futura malla continua; no se derivan de una seed.

## Rutas obligatorias

| Ruta | Recorrido | Decisión | Resultado esperado |
| --- | --- | --- | --- |
| Directa, expuesta | Ítaca → fibra → ferrita → cresta → señal | Acorta camino, pasa cerca del nido este | Encuentro evitable con Umbra Grazer; señal visible como guía. |
| Rodeo, segura | Ítaca → fibra → cristal → corredor sur → flanco este → señal | Recorre más, reduce exposición a nidos | Reúne el recurso característico y permite aproximación más controlada. |
| Opcional | Ítaca → anomalía oeste → regreso | Añade evidencia de bitácora, sin bloquear misión | Descubrimiento extra y landmark de retorno. |

Los recursos críticos se encuentran en ambas lecturas de la zona; las vetas profundas y los
nidos no pueden bloquearlos. Cada ruta debe tener un ancho útil mínimo de 3 m, sin contar
decoración, y pendientes/colliders aptos para la cápsula de 84 cm de diámetro.

## Zonas reservadas

- **Landing/refugio:** radio de 30 m alrededor de Ítaca. Sin PCG bloqueante, nidos ni roca
  alta; la escotilla y el aterrizaje conservan espacio libre.
- **Corredores críticos:** bandas de 3 m sobre las dos rutas. PCG sólo puede colocar filler
  no bloqueante; recursos secundarios y fauna deben respetar una exclusión mayor.
- **POI principal:** radio de 15 m alrededor de la señal para lectura, interacción y final.
- **POIs secundarios:** anomalía y vetas con acceso lateral, nunca dentro de un corredor.

## Variación secundaria

Las seeds `100`, `200`, `300`, `400` y `500` se usarán para probar densidad, escala, rotación,
material y agrupamiento de rocas/flora/filler, además de encuentros menores. No pueden cambiar
ninguna coordenada de esta hoja, la landing zone, los recursos críticos ni los POIs principales.

## Criterios antes de integrar arte/PCG

1. Las dos rutas alcanzan la señal y vuelven a Ítaca sin rescate.
2. Un nido amenaza, pero no ataca a través de obstáculos ni vuelve obligatoria la muerte.
3. La señal, Ítaca y la cresta se distinguen a altura de jugador.
4. Los cinco seeds secundarios producen variación visible sin invadir exclusiones.
5. El mapa revelado conserva los recorridos y POIs descubiertos tras guardar/cargar.
