# Humano, manos y animaciones — primer lote Blender

2026-09-06. Pedido autorizado: avanzar con el primer lote de personaje e incluir
animaciones simples. Calidad **Q1**, original y reproducible, no arte final.

## Entrega

- `SKEL_Humanoid_A`: 57 huesos, raíz al suelo, columna, cabeza, extremidades,
  tres articulaciones por dedo y cuatro anclajes de equipo. Pose de referencia A.
- `SK_Human_Body_Blockout`: explorador vestido de 1,80 m, 9.256 triángulos,
  cuatro materiales. Casco, visor, placas, botas y mochila son piezas de prototipo
  incluidas en la misma malla; aún no son un catálogo de ropa intercambiable.
- `SK_Human_HandsFP_Blockout`: dos guantes y antebrazos, 4.336 triángulos,
  dos materiales; mismo rig y articulaciones que el cuerpo.
- Siete acciones a 30 FPS y un FBX por acción, además de dos FBX de malla.

| Acción | Duración | Uso |
| --- | --- | --- |
| `AN_Human_Idle_Blockout` | 3 s | Respiración y movimiento leve de cabeza; cíclica |
| `AN_Human_Walk_Blockout` | 1 s | Marcha en el sitio; cíclica |
| `AN_Human_Run_Blockout` | 0,667 s | Carrera en el sitio; cíclica |
| `AN_Human_Jump_Blockout` | 1 s | Flexión y gesto de salto; desplazamiento vertical a cargo del Character |
| `AN_Human_Land_Blockout` | 0,6 s | Amortiguación y recuperación |
| `AN_Human_Interact_Blockout` | 1,2 s | Alcanzar con brazo derecho y retirar |
| `AN_Human_Grip_Blockout` | 1 s | Cerrar y abrir dedos; prueba de agarre, no agarre específico de herramienta |

Tecnología humana reparable: tejido gris azulado, carcasas claras, visor oscuro y
acento cian. Las articulaciones usan superficies de anillos con pesos mezclados;
las protecciones rígidas siguen el hueso correspondiente. UV0 por islas para prototipo,
sin texturas externas, física por dedos, tela ni rig facial.

## Archivos para revisar

Carpeta: `ContentPipeline/Generated/HumanoidBlockout/`.

- `Humanoid_Blockout_Source.blend`: fuente canónica con las siete acciones; al abrir
  tiene `Walk` activo. Seleccionar `SKEL_Humanoid_A`, cambiar acciones en el Action Editor
  y ajustar el rango según la tabla. Los fotogramas incluyen ambos extremos del ciclo.
- `Humanoid_Blockout_Preview.blend`: escenario iluminado con cámara, caminata activa.
- `Humanoid_Animation_Review.blend`: revisión preparada de dos ciclos de marcha y
  tres de carrera, fotogramas 1–120. Reproducir con Espacio; el cambio a carrera es en 61.
  Esta secuencia de revisión tiene un corte entre acciones; no es un BlendSpace.
- `Humanoid_Walk_Run.mp4`: video de revisión de 4 segundos, 512×576.
- `Preview_Idle.png`, `Preview_Walk.png`, `Preview_Run.png`,
  `Preview_Hands_Open.png`, `Preview_Hands_Grip.png`: imágenes de estudio 900×1000.
- Nueve FBX: dos mallas y siete clips. No exportar el piso, las luces ni la cámara.

Las manos FP están ocultas en las escenas de cuerpo para evitar duplicación. Para
editarlas, ocultar la malla corporal y mostrar `SK_Human_HandsFP_Blockout` en el Outliner.
Estos archivos generados son locales e ignorados por Git; el código y la configuración
son la fuente reproducible. No editar manualmente un generado y regenerarlo encima
sin antes preservar esa edición como una fuente independiente.

## Reproducción y pruebas

Desde la raíz del repositorio, en PowerShell:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/Blender/generators/humanoid_blockout.py
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -run=pythonscript -script='C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\ValidateHumanoidImport.py' -unattended -nullrhi -nosplash -nop4 -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\HumanoidImportVerified.log'
& 'C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe' Tools/Blender/build_humanoid_manifest.py
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/Blender/render_humanoid_preview.py
```

El importador requiere acceso a la caché normal Zen de Unreal; se autorizó su ejecución
fuera del sandbox. Importa sólo en memoria bajo `/Game/Astraeon/ArtValidation/HumanoidBlockout`;
rechaza un destino existente. No guarda paquetes `.uasset` ni mapas.

Validaciones efectuadas:

- Escala, raíz única, jerarquía y anclajes, geometría cerrada por pieza, UV,
  materiales, presupuesto y pesos normalizados con máximo cuatro influencias.
- Cuatro controles negativos: escala sin aplicar, peso sin normalizar, vértice sin peso
  y anclaje ausente. Todos rechazados.
- Dos generaciones de geometría comparadas: resultado semántico idéntico. La enumeración
  no estable de caras del operador UV sphere se normaliza sin cambiar su orientación.
- 261 fotogramas, comprobando deformación finita en ambas mallas, movimiento real,
  ausencia de root motion y cierre exacto de los tres ciclos en la fuente.
- Contacto de la bota inferior dentro de 1 mm del suelo en marcha, carrera y aterrizaje.
- Nueve roundtrips FBX: jerarquía, escala, skin y duración/movimiento conservados.
- Unreal 5.7.4: dos SkeletalMesh sobre el mismo Skeleton y siete AnimSequence,
  cuerpo de 180 cm; evaluación de poses, movimiento y cierre de ciclos. 57 huesos.

Evidencia: `ContentPipeline/reports/humanoid_blockout_validation.json`,
`humanoid_unreal_import.json`, `humanoid_video.json` y
`ContentPipeline/humanoid_asset_manifest.json`. El manifiesto rechaza fuentes/FBX
modificados después de la validación.

Durante exportación el objeto armature se llama temporalmente `Armature`: el importador
FBX de UE 5.7 elimina explícitamente ese contenedor de Blender. Así se conserva `root`
como raíz real; la fuente sigue llamándose `SKEL_Humanoid_A`. No implica compatibilidad
con Manny/Quinn, que no fue ensayada.

## Límites y siguiente trabajo

La importación comprobada **no conecta estas animaciones a controles**. No se cambió
C++, el mapa, la cápsula, los guardados ni la build jugable. No corresponde atribuir a
este lote compilación nueva, smoke jugable, rendimiento ni aceptación Q2.

Pendientes: ajuste de cámara/manos FP con la herramienta real, animación de escáner,
cortadora/taladro, transiciones y mezcla por velocidad, IK de pies y reducción de
deslizamiento, revisión de solapes en puños/protecciones, guantes más anatómicos,
materiales de producción y componentes de equipo intercambiables. La carrera Q1 mantiene
un apoyo; aún no tiene una fase de vuelo físicamente refinada. El salto es un gesto
en el sitio, no una trayectoria balística animada.

Siguiente lote artístico: escáner, cortadora y taladro ajustados a estos agarres;
después martillo, maza y ración. La conexión visual al Character debe verificarse
con cámara real, cambios de herramienta y estados de locomoción antes de marcar
personaje/animación como integrados.
