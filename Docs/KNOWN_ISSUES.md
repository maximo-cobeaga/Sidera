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

### Falta smoke visual/manual

- **Síntoma**: el slice se validó con build, tests, commandlet y smoke headless, pero no con una sesión humana/visual.
- **Estado**: pendiente.
- **Impacto**: medio. La escala, legibilidad de labels, claridad de HUD e interacción pueden requerir ajustes.
- **Acción recomendada**: ejecutar `Builds/WindowsDevelopment/Astraeon.exe` sin `-nullrhi` y completar el recorrido manualmente.

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
