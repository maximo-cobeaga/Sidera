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

### Interacción (`E`) sin mira ni feedback notorio (resuelto en Rebuild4)

- **Síntoma original**: presionar `E` no producía efecto visible; recursos difíciles de recolectar.
- **Estado**: resuelto en Rebuild4. Crosshair central en HUD, línea `Estado:` en verde, rocas de terreno no bloquean `ECC_Visibility`. Pendiente confirmación manual.
- **Acción pendiente**: jugar el build y confirmar con la mira que `E` sobre ESCOTILLA y RECURSO produce feedback verde visible.

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
