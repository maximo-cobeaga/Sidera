# Calibración Blender → Unreal — `UE57_AST_V1`

Fecha: 2026-09-09. Cierra la calibración de la puerta de la **Fase 0**
([ADR 0004](../ADR/0004-planetas-esfericos-fundacionales.md)).

Responde una pregunta con números: **¿un metro de Blender llega a Unreal como 100 cm, con los ejes
en su sitio, para las cuatro clases de asset que el proyecto usa?**

Por qué hace falta aunque el pipeline ya funcionara: el proyecto ya pagó este error una vez. La
importación de animaciones sueltas perdía la escala 100 del Armature y el protagonista quedaba
invisible ([INVESTIGACION_PERSONAJE_INVISIBLE.md](../INVESTIGACION_PERSONAJE_INVISIBLE.md)). Esto
convierte ese fallo en algo que se detecta en dos minutos.

---

## Resultado: **PASA**

| Comprobación | Esperado | Medido | |
|---|---:|---:|---|
| Cubo de 1 m, eje X | 100 cm | 100,000 cm | ✓ |
| Cubo de 1 m, eje Y | 100 cm | 100,000 cm | ✓ |
| Cubo de 1 m, eje Z | 100 cm | 100,000 cm | ✓ |
| Eje X (lado largo) | 50 cm | 50,000 cm | ✓ |
| Eje Y (lado largo) | 35 cm | 35,000 cm | ✓ |
| Eje Z (lado largo) | 20 cm | 20,000 cm | ✓ |
| Mannequin, altura | 183 cm | 183,000 cm | ✓ |
| Morph target del shape key | `Calibration_Widen` | `Calibration_Widen` | ✓ |
| Animación, duración | ~1,0 s | 1,000 s / 30 frames | ✓ |

**Los tres ejes miden distinto a propósito.** Con tres ejes iguales, una permutación de ejes en el
FBX sería indetectable: el resultado seguiría pareciendo correcto. Con 50/35/20 cm, cualquier
intercambio se lee en los números.

---

## Entorno registrado

| | |
|---|---|
| Blender | 5.2.1 LTS (build 2026-08-25), Python 3.13.13 |
| Unreal | 5.7.4-51494982 |
| SO | Windows 11 (10.0.26200) |
| Preset | `UE57_AST_V1` v1 |
| Escena | Métrica, Unit Scale 1.0, 30 FPS |

Preset congelado: forward `-Y`, up `Z`, global scale 1.0, apply unit scale, sin leaf bones,
ejes de hueso Y/X, simplify de animación 0.0.

---

## Cómo reproducirlo

```powershell
# 1. Blender genera la escena y exporta con el preset
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background `
    --python .\Tools\Blender\calibration_ue57.py

# 2. Unreal importa, mide y arma TL_00_AssetCalibration
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
    .\Astraeon.uproject -unattended -nop4 -nosplash -RenderOffscreen `
    -ExecutePythonScript=.\Scripts\Editor\CreateAssetCalibrationMap.py
```

Ambos devuelven código distinto de cero si algo falla y dejan un JSON en `Docs/evidencia/`.

---

## El preset dejó de estar duplicado

Los ajustes de exportación vivían **en dos sitios**: `Tools/Blender/exporters/fbx.py` y
`Tools/Blender/main_character_export.py`. Coincidían, pero nada lo garantizaba, y cambiar uno sin
el otro produce assets con ejes o escala distintos según por qué exportador pasaron.

Ahora la fuente única es `Tools/Blender/presets/UE57_AST_V1.py`. `exporters/fbx.py` lo consume, y
la calibración **comprueba en cada corrida** que `main_character_export.FBX_COMMON` sigue siendo
idéntico al preset:

```json
"preset_drift_check": { "checked": true, "differences": {} }
```

Si alguien cambia uno y no el otro, la calibración falla ahí, y no cuando un personaje aparezca
rotado en Unreal tres sesiones después.

---

## Tres cosas que se aprendieron construyéndolo

**Interchange ignora las opciones de `FbxImportUI`.** Es el importador por defecto en 5.7 y con un
FBX de sólo armature responde *"no había ningún dato que importar"*. Los importadores del
protagonista y de la criatura ya desactivaban Interchange con
`Interchange.FeatureFlags.Import.FBX 0`; la calibración tiene que medir **el mismo camino que usan
los assets de verdad**, así que hace lo mismo.

**Una calibración no puede reimportar sobre lo que dejó la corrida anterior.** El primer intento
lo hacía y perdió los morph targets: el informe habría culpado al exportador de un defecto que
estaba en el estado previo. Ahora se limpia el destino antes de importar, y el borrado tolera una
carpeta a medias porque una corrida interrumpida deja assets huérfanos.

**`pose_bone.location` está en espacio local del hueso**, y en Blender el eje local Y corre a lo
largo del hueso. Animar Z en un hueso vertical lo mueve de costado, no hacia arriba. La primera
versión del script hacía eso y la comprobación de animación fallaba con la animación bien.

---

## Archivos

- `calibration_blender_20260909_191855.json` — lado Blender: versiones, preset, escena, drift check
- `calibration_unreal_20260909_192357.json` — lado Unreal: medidas e importación
- `Content/Maps/TL_00_AssetCalibration.umap` — el test level, con cubo y mannequin colocados
- `Content/Astraeon/Test/Calibration/` — los seis assets importados
