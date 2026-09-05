# Problemas conocidos — ASTRAEON

## Baja severidad

### NullRHI automation bootstrap logs `Condition failed`

- **Síntoma**: durante el arranque de `UnrealEditor-Cmd.exe -nullrhi`, antes de ejecutar la suite `Astraeon.*`, aparecen tres líneas:
  - `LogAutomationTest: Error: Condition failed`
- **Estado**: observado de forma recurrente.
- **Impacto**: bajo. La cola propia `Astraeon.*` termina exit code 0 y todos los tests del proyecto pasan.
- **Acción recomendada**: investigar sólo si empieza a afectar exit code, packaging o pruebas del proyecto.

### PowerShell no reporta exit code confiable para smoke packaged

- **Síntoma**: `Astraeon.exe -nullrhi ...` termina y no queda proceso vivo, pero `$LASTEXITCODE` aparece vacío en algunas corridas.
- **Estado**: observado en smoke simple y critical-path smoke packaged.
- **Impacto**: bajo-medio. La evidencia de log confirma inicialización/salida limpia y recorrido crítico exitoso.
- **Acción recomendada**: crear wrapper de smoke con timeout/proceso explícito o comando interno que escriba un sentinel file.

### Falta smoke visual/manual (caída de mapa: CONFIRMADA RESUELTA)

- **Síntoma original**: al presionar `E` sobre `SURFACE HATCH` en juego real, el jugador caía del mapa.
- **Estado**: **resuelto y confirmado por el usuario** jugando el build recompilado (2026-09-05): "Ya no me caigo del mapa". El fix (piso único vía `MaterializeCurrentRegion`, reintento de materialización si falta piso, y red de rescate anti-caída general — ver `DECISIONS.md`) quedó validado con una sesión manual real, no sólo con el smoke automatizado.
- **Impacto**: cerrado. Se mantiene la red de rescate anti-caída como protección general para el resto del juego.
- **Seguimiento**: dos observaciones nuevas del usuario tras confirmar el fix — ver la entrada siguiente ("Interacción sin mira ni feedback notorio").

### Interacción (`E`) sobre ESCOTILLA no registra — trace no alcanza el cubo

- **Síntoma**: presionar `E` apuntando a la ESCOTILLA muestra "Sin consola ARGOS, escotilla, recurso recolectable ni fuente de señal al alcance." — el feedback correcto de despliegue nunca aparece.
- **Estado**: confirmado por el usuario en Rebuild4 (2026-09-05). El crosshair y el verde en `Estado:` ya están presentes, pero la interacción no ocurre.
- **Diagnóstico**: la causa más probable es que el hitbox de la ESCOTILLA es demasiado pequeño para alcanzarlo con un trace de punto único. El cubo está en `FVector(620, 0, 80)` con escala `(1.2, 0.5, 0.35)` → área frontal efectiva de solo **50 cm × 35 cm**. El jugador debe mirar levemente hacia abajo (~7° desde horizontal) y apuntar al centro exacto. Un trace de línea sin sweep no tiene margen de error.
- **Causa secundaria posible**: el jugador llega a la ESCOTILLA sin haber interactuado primero con ARGOS; aunque el código no lo bloquea, el flujo de hints podría no estar apuntando a la escotilla todavía.
- **Impacto**: alto — el primer punto de interacción obligatorio del juego no funciona de forma confiable.
- **Solución recomendada para próxima sesión**: reemplazar el line trace puro en `Interact()` por una detección de proximidad esférica como fallback. Si ningún marker está en el rango del trace directo, hacer un `OverlapMultiByChannel` en radio de 180 cm centrado en el jugador y seleccionar el marker más cercano. Esto hace que acercarse a cualquier marcador y presionar `E` funcione sin necesidad de apuntar con precisión milimétrica. Alternativamente, aumentar el scale Z de la ESCOTILLA a ≥1.0 para que sea un blanco más fácil de alcanzar.
- **Archivos a modificar**: `AstraeonPlayerCharacter.cpp` → `Interact()`, y/o `AstraeonRegionMaterializer.cpp` → `BuildItacaActorSpecs()` (scale del hatch).

### Vista mayormente negra al alejarse del punto de despliegue (fix incluido en Rebuild4)

- **Síntoma**: caminando unos pocos pasos desde el spawn/hatch, la vista 3D pasa a verse casi completamente negra; sólo el HUD de texto sigue siendo legible.
- **Estado**: fix incluido en Rebuild4 (2026-09-05), smoke crítico pasó. `AExponentialHeightFog` (densidad 0.04, start 800 cm, falloff 0.15) + sol con tinte dorado-cálido + 12 rocas procedurales ocre/naranja + colores en suelo y pad. Pendiente confirmación visual manual.
- **Impacto**: reducido — la bruma reemplaza el fondo vacío. Pendiente smoke manual para confirmar.
- **Acción pendiente**: jugar el build y confirmar que alejarse del pad muestra bruma en vez de negro.

### Git Bash convierte rutas Unreal `/Game/...`

- **Síntoma**: al ejecutar `UnrealEditor-Cmd.exe ... /Game/Maps/L_AstraeonBootstrap` desde Git Bash, MSYS puede convertir el argumento a `C:/Program Files/Git/Game/Maps/L_AstraeonBootstrap`, causando fallo de carga y crash posterior.
- **Estado**: reproducido y evitado.
- **Impacto**: bajo si se usa PowerShell o `MSYS_NO_PATHCONV=1`.
- **Acción recomendada**: para comandos Unreal desde Git Bash con rutas `/Game/...`, prefijar `MSYS_NO_PATHCONV=1`.

### `CreateBootstrapMap.py` no regenera sobre mapa existente

- **Síntoma**: `EditorLevelLibrary.new_level` falla si `/Game/Maps/L_AstraeonBootstrap` ya existe; en UE 5.7 commandlet derivó en crash al intentar esa recreación.
- **Estado**: reproducido una vez durante esta iteración; no afecta el flujo runtime ni el smoke de mapa existente.
- **Impacto**: bajo. Sólo afecta regenerar el `.umap` desde script.
- **Acción recomendada**: antes de usar el script como regenerador, reemplazarlo por una rutina explícita y verificada de cargar/limpiar o crear con autorización sobre el asset binario.

## Deuda técnica aceptada temporalmente

### HUD y menú C++ temporales

- **Motivo**: acelerar verificación mecánica del MVP antes de UI final.
- **Impacto**: presentación pobre, pero lógica testeable.
- **Cierre esperado**: reemplazar por UI Blueprint/UMG delgada cuando el flujo se estabilice.

### Marcadores con `TextRenderComponent`

- **Motivo**: diferenciar recursos, ARGOS, anomalía y señal sin assets externos.
- **Impacto**: útil para debug/jugabilidad temprana, no arte final.
- **Cierre esperado**: reemplazar por meshes/materiales/FX propios o licenciados.

### Ítaca/ARGOS incompleto visualmente

- **Motivo**: existe consola ARGOS funcional, pero no una estancia de nave presentable ni transición narrativa pulida.
- **Impacto**: el flujo cumple mecánicamente, no todavía como experiencia narrativa final.
- **Cierre esperado**: crear zona inicial de Ítaca, lectura clara de briefing y transición controlada a región.
