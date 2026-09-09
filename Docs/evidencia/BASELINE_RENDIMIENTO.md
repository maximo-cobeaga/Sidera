# Baseline de rendimiento — antes de la transición planetaria

Fecha: 2026-09-09. Cierra el punto "medir baseline" de la puerta de la **Fase 0**
([ADR 0004](../ADR/0004-planetas-esfericos-fundacionales.md),
[PLAN_TRANSICION_EJECUCION.md](../PLAN_TRANSICION_EJECUCION.md) §4).

Para qué sirve: es el punto de comparación de las Fases 1 y 2. Cuando el planeta por patches
empiece a generar malla y colisión en runtime, la pregunta será *cuánto costó*, y sin un número
anterior esa pregunta no tiene respuesta.

---

## Cómo reproducirlo

```powershell
$exe = 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
& $exe .\Astraeon.uproject /Game/Maps/L_AstraeonBootstrap -game -unattended -nop4 -nosplash `
    -RenderOffscreen -windowed -ResX=1920 -ResY=1080 `
    -AstraeonPerfBaseline -AstraeonPerfWarmup=10 -AstraeonPerfSeconds=30
```

Escribe un JSON con marca de tiempo en `Docs/evidencia/`. `-AstraeonPerfBaseline` arranca partida,
descarta el calentamiento, muestrea, escribe el informe y cierra el proceso.

El calentamiento no es un adorno: los primeros segundos miden compilación de shaders y carga de
streaming, no el juego, y meterlos en la muestra arruina cualquier percentil.

---

## Resultados

Ambos mapas, 30 s de muestreo tras 10 s de calentamiento, 1920 × 1080, Development.

| Métrica | `L_AstraeonBootstrap` (mundo plano) | `TL_10_RadialGravity` (harness) | Presupuesto (`AGENTS.md` §8) |
|---|---:|---:|---|
| Frames muestreados | 8.694 | 8.972 | — |
| FPS medio | **289,8** | **299,0** | ≥ 60 |
| Mediana de frame | 3,37 ms | 3,32 ms | — |
| P99 de frame | 5,19 ms | 4,50 ms | ≤ 22,2 ms (45 FPS) |
| Peor frame | 18,62 ms | 7,61 ms | — |
| Hitches > 50 ms | **0** | **0** | ninguno recurrente |
| Memoria física | 2.349 → 2.362 MB | 2.312 → 2.301 MB | sin crecimiento ilimitado |

**Lectura.** Los dos mapas están un orden de magnitud por encima del presupuesto, con margen
enorme. No es mérito: el mundo plano son baldosas instanciadas y el harness es una esfera. El
valor del número no es que sea alto, es que **existe y es reproducible** — cuando la Fase 2 meta
generación de patches en workers, la comparación será contra estas cifras exactas.

El único dato que ya dice algo: el peor frame del mundo plano (18,6 ms) es 2,4× el del harness.
Es coherente con que `MaterializeCurrentRegion()` reconstruya la región entera de golpe, que es
justo lo que el streaming por patches viene a reemplazar.

Memoria estable en ambos: +12 MB y −11 MB en 30 s, dentro del ruido del recolector.

---

## Qué NO mide esto, y hay que tenerlo presente

- **Render offscreen, no ventana real.** Comparable consigo mismo, no con lo que ves jugando.
- **Sesión quieta.** Nadie camina, escanea ni fabrica: no hay carga de gameplay ni de streaming
  por movimiento. La medida de la Fase 2 tendrá que mover al jugador o no medirá lo que importa.
- **Ni GPU ni VRAM.** El presupuesto pide margen en una GPU de 8 GB y esto no lo cubre. Hace falta
  Unreal Insights, que la puerta de la Fase 2 ya exige.
- **Editor, no build empaquetada.** La build etiquetada `pre-transicion-plana` es anterior a esta
  herramienta, así que no puede medirse con ella sin reempaquetar — y reempaquetar la convertiría
  en otra build. Se mide el editor a propósito, y queda dicho.
- **El código de la transición está presente pero dormido** en el mapa plano: sin harness,
  `UAstraeonPlanetGravityComponent` no se activa. Por eso la cifra de `L_AstraeonBootstrap` sigue
  representando el estado previo.

---

## Evidencia adicional capturada de paso

La corrida sobre `TL_10_RadialGravity` dejó en el log la prueba de que el enganche funciona
en una sesión de juego real, no sólo en pruebas unitarias:

```
LogAstraeonDiag: [Planet] Harness activo: centro=X=0.000 Y=0.000 Z=0.000 radio=20000 cm (0.2 km) g=9.81 m/s2
LogAstraeonDiag: [Planet] Gravedad radial activa sobre centro=X=0.000 Y=0.000 Z=0.000 radio=20000 cm
```

El componente encontró el harness, tomó su centro y su radio y fijó la dirección de gravedad.
**Esto no cierra el criterio de orientación de la puerta**, que exige ver al personaje de pie en
el antípoda sin que la cámara ruede, y eso sigue necesitando una partida humana.

---

## Archivos

- `perf_baseline_20260909_191050.json` — `L_AstraeonBootstrap`
- `perf_baseline_20260909_191153.json` — `TL_10_RadialGravity`

Cada JSON incluye su línea de comandos completa y la configuración de build. Una medida sin sus
condiciones es una anécdota.
