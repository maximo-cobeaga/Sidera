# Corrección de animación del protagonista — 2026-09-10

## Resultado

Se corrigieron dos problemas visibles reportados durante la prueba del personaje:

1. Los brazos quedaban demasiado cerca del tórax y las hombreras.
2. La locomoción podía cambiar de clip de forma brusca y parecer interrumpida antes de completar
   el ciclo.

La corrección se realizó sobre la escena canónica de Blender y se integró en Unreal Engine 5.7.4.
No se consumieron créditos Higgsfield.

## Diagnóstico

La imagen recibida mostraba una silueta frontal con poco espacio negativo entre brazos, torso y
hombreras. La inspección de la escena indicó que el problema era principalmente de pose del rig,
no una unión topológica irrecuperable de la malla: el `upperarm` tenía una rotación base demasiado
cerrada para el volumen del traje.

El ciclo `AN_Player_Walk_F` no tenía una costura rota: su rango era `1–37`, a 30 FPS, y la pose del
primer y último frame coincidía. El síntoma de corte venía del selector runtime, que podía cambiar
entre `Idle`, `Walk`, `Run` o direcciones al cruzar umbrales de velocidad y reiniciar el asset en
el frame inicial.

## Cambios realizados en Blender

Archivo canónico:

`graphics/characters/main_player/blender/CHR_Astraeon_Player.blend`

Se fijó una pose A funcional con `upperarm X = -1,08 rad` en:

- `AN_Player_Idle`
- `AN_Player_Walk_F/B/L/R`
- `AN_Player_Run_F/B/L/R`

Esto separa visualmente brazos, hombreras y tórax sin convertir la silueta en una pose T. Se
conservaron el esqueleto, la cadencia, los rangos de frames, las curvas Bézier y la animación
in-place.

El ciclo frontal de caminar quedó verificado con estos datos:

| Propiedad | Resultado |
|---|---:|
| Rango | 1–37 |
| Frecuencia | 30 FPS |
| Duración | 1,2 s |
| Costura inicio/final | 0,0 de diferencia máxima |
| Curvas modificadas | Bézier |

Evidencia visual:

- `Saved/BlenderRecovery/walk_arm_silhouette_final_f01.png`
- `Saved/BlenderRecovery/walk_arm_silhouette_final_f10.png`
- `Saved/BlenderRecovery/walk_arm_silhouette_final_f20.png`
- `Saved/BlenderRecovery/walk_arm_silhouette_final_f37.png`

Se guardó un checkpoint anterior al ajuste:

`Saved/BlenderRecovery/CHK_Player_before_arm_silhouette_walk_cycle_20260910.blend`

## Cambios realizados en Unreal

Código principal modificado:

`Source/Astraeon/Private/Presentation/AstraeonFirstPersonRigComponent.cpp`

Se añadieron:

- Histeresis de locomoción: entrada a caminar a 16 cm/s y permanencia hasta 6 cm/s.
- Permanencia en carrera hasta 560 cm/s; entrada a carrera a 620 cm/s.
- Conservación de fase normalizada al cambiar entre ciclos locomotores.
- Evitación del reinicio constante del mismo asset.

La variante corporal corregida se importó en:

`/Game/Astraeon/Characters/Player/Optimized_Polished`

La variante anterior `/Game/Astraeon/Characters/Player/Optimized` permanece disponible para
comparación y rollback. El FBX previo se conservó como:

`graphics/characters/main_player/exports/AN_Astraeon_Player_All_Polished_20260910_before_arm_pose.fbx`

Las manos de primera persona continúan usando la variante aislada:

`/Game/Astraeon/Art/Blockouts/Human/PolishedFP`

## Verificación

| Verificación | Resultado |
|---|---|
| Build `AstraeonEditor Win64 Development` | PASS |
| Automation | PASS — 75/75 |
| Critical | PASS |
| Package Development | PASS |
| PackagedCritical | PASS |
| PackagedVisual | PASS |
| Importación de animaciones | PASS — 51 secuencias |
| Validación de manos FP | PASS |
| Créditos Higgsfield consumidos | 0 |

Reportes:

- `ContentPipeline/reports/polished_character_animation_import.json`
- `ContentPipeline/reports/polished_first_person_hands_validation.json`
- `graphics/characters/main_player/docs/locomotion_polish_20260910.json`
- `Saved/Logs/Character_Automation.log`
- `Saved/Logs/Character_Critical.log`
- `Saved/Logs/Character_Package.log`
- `Saved/Logs/Character_PackagedCritical.log`
- `Saved/Logs/Character_PackagedVisual.log`

## Cómo probarlo

### Desde el editor

1. Abrir `Astraeon.uproject` con Unreal Engine 5.7.4.
2. Abrir el mapa `L_AstraeonBootstrap`.
3. Ejecutar Play In Editor.
4. Presionar `Enter` si aparece la pantalla inicial.
5. Probar `WASD`, `Shift` para correr, `Space` para saltar y `V` para alternar primera/tercera
   persona.

La prueba recomendada es mantener `W` varios segundos, activar/desactivar `Shift`, cambiar entre
`W`, `A` y `D`, y soltar la entrada. La inspección principal debe hacerse en tercera persona.

### Desde el build

Ejecutable:

`Builds/WindowsProtagonista/Astraeon.exe`

El build fue regenerado después de la reimportación de las animaciones y pasó los checks críticos
empaquetados.

## Pendiente

La corrección queda integrada, pero todavía falta una prueba humana sostenida para medir:

- contacto y patinaje de pies contra la velocidad real de `CharacterMovement`;
- cambio de dirección caminando y corriendo;
- transición de movimiento a detención;
- percepción de la animación en primera y tercera persona.

La mezcla completa mediante `AnimBlueprint`/`BlendSpace` no forma parte de esta corrección y queda
como mejora posterior si la prueba humana todavía detecta saltos entre estados.
