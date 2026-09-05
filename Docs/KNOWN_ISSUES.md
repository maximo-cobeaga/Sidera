# Problemas conocidos — ASTRAEON

## Pendiente de verificación manual

### Interacción (`E`) sobre ESCOTILLA/recursos — fix automatizado, falta smoke real

- **Síntoma original**: presionar `E` apuntando a la ESCOTILLA podía mostrar "Sin consola ARGOS, escotilla, recurso recolectable ni fuente de señal al alcance."; el usuario también reportó que un recurso escaneado no se recolectaba con `E`.
- **Diagnóstico**: el line trace de interacción era demasiado estricto para marcadores temporales pequeños. La ESCOTILLA mide sólo ~50×35 cm de frente y requería apuntar con precisión al centro del cubo.
- **Fix aplicado (2026-09-05 Rebuild5)**: `AAstraeonPlayerCharacter::Interact()` mantiene el trace directo, pero si no encuentra un `AAstraeonRegionMarker` usa fallback por proximidad de 180 cm y selecciona el marcador más cercano. El smoke crítico ahora reproduce la falla manual apuntando horizontalmente por encima de la ESCOTILLA y aun así pasa.
- **Evidencia automática**: editor-game y packaged smoke reportan `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- **Acción pendiente**: smoke manual del ejecutable sin `-nullrhi` para confirmar sensación real de uso: acercarse a ESCOTILLA/recurso y presionar `E` sin apuntado perfecto.

### Vista mayormente negra al alejarse del punto de despliegue — fix incluido, falta confirmación visual

- **Síntoma**: caminando unos pocos pasos desde el spawn/hatch, la vista 3D pasaba a verse casi completamente negra; sólo el HUD era legible.
- **Estado**: fix incluido desde Rebuild4/Rebuild5: `AExponentialHeightFog` runtime, sol con tinte dorado-cálido, rocas procedurales ocre/naranja, colores en suelo y pad.
- **Impacto**: reducido por automatización, pero la calidad visual final depende de prueba humana.
- **Acción pendiente**: jugar el build y confirmar que alejarse del pad muestra bruma/terreno en vez de negro plano.

### Save/close/open/continue no validado manualmente

- **Síntoma**: save/load pasa automáticamente, pero todavía falta una prueba humana del flujo `F5` → cerrar ejecutable → abrir → `F9`.
- **Estado**: pendiente.
- **Impacto**: medio para H6, porque AC-11 exige continuar correctamente desde una sesión real.

### Rendimiento no medido en sesión jugable

- **Síntoma**: no existe medición interactiva de FPS/frametime del build actual.
- **Estado**: pendiente.
- **Impacto**: medio para H6/AC-14.
- **Acción recomendada**: después del smoke manual, registrar FPS promedio aproximado a 1080p en calidad por defecto y observar stutter/memoria.

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

### Git Bash convierte rutas Unreal `/Game/...`

- **Síntoma**: al ejecutar `UnrealEditor-Cmd.exe ... /Game/Maps/L_AstraeonBootstrap` desde Git Bash, MSYS puede convertir el argumento a `C:/Program Files/Git/Game/Maps/L_AstraeonBootstrap`, causando fallo de carga y crash posterior.
- **Estado**: reproducido y evitado.
- **Impacto**: bajo si se usa PowerShell o `MSYS_NO_PATHCONV=1`.
- **Acción recomendada**: para comandos Unreal desde Git Bash con rutas `/Game/...`, prefijar `MSYS_NO_PATHCONV=1`.

### `CreateBootstrapMap.py` no regenera sobre mapa existente

- **Síntoma**: `EditorLevelLibrary.new_level` falla si `/Game/Maps/L_AstraeonBootstrap` ya existe; en UE 5.7 commandlet derivó en crash al intentar esa recreación.
- **Estado**: reproducido una vez durante una iteración previa; no afecta el flujo runtime ni el smoke de mapa existente.
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
