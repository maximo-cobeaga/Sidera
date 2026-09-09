# Arte regional — entrega Q1, 2026-09-06

Siete mallas originales sustituyen la presentación cúbica de los recursos y puntos de
interés de §3.4 de `ASSETS_PENDIENTES_DISENO_ANIMACION.md`. El bloque está integrado y
verificado en editor-game y ejecutable Development. No entrega manos, rigs, animaciones,
audio, VFX, UMG, arte final ni nuevas mecánicas.

## Probar la entrega

Ejecutar `Builds/WindowsRegionArt/Astraeon.exe`. `Enter` inicia partida, `E` interactúa,
click izquierdo escanea, `C` fabrica el resonador, `F6` guarda y `F10` continúa desde menú.
Las teclas y recetas existentes permanecen. Mirar fibra, ferrita, recurso regional,
las dos vetas profundas, señal y anomalía. Las vetas conservan el requisito de taladro y
ahora lo explican correctamente al presionar `E` sin herramienta.

BuildCookRun Win64 Development: exit 0, `BUILD SUCCESSFUL`. El destino separado conserva
los paquetes previos. Suite `Astraeon.*`: **46/46**. Smokes editor-game y packaged de
recorrido crítico y arte: aprobados. Reportes en `ContentPipeline/reports/region_*.json`.
No se realizó una prueba desde checkout limpio: hay cambios ajenos previos y dependencias
de integración todavía sin commit. No se mezclaron en un commit de esta entrega.

## Contenido y límites

| Asset | Presentación | Dimensiones (cm) |
| --- | --- | --- |
| `SM_Resource_SilicateFiber_Blockout` | Haz de filamentos | 80 × 80 × 100 |
| `SM_Resource_FerriteNodule_Blockout` | Nódulos facetados | 80 × 80 × 100 |
| `SM_Resource_SeedCrystal_Blockout` | Grupo cristalino provisional compartido | 110 × 110 × 115 |
| `SM_Resource_CryoVein_Blockout` | Collar mineral con nódulos retraídos | 110 × 110 × 115 |
| `SM_Resource_QuartzVein_Blockout` | Collar mineral con cristales retraídos | 110 × 110 × 115 |
| `SM_Signal_Source_Blockout` | Anillos cruzados y núcleo | 160 × 160 × 220 |
| `SM_Anomaly_Strata_Blockout` | Estratos girados | 100 × 100 × 140 |

Destino: `/Game/Astraeon/Art/Blockouts/Region`. Un slot/material por mesh, menos de 1200
triángulos cada uno, UV de depuración, sin texturas externas. Geometría original local.
El manifiesto `ContentPipeline/region_asset_manifest.json` registra dimensiones, hashes,
procedencia y evidencia. El catálogo principal lo enlaza sin reemplazar sus familias previas.

`AAstraeonRegionMarker` mantiene ids, requisitos, transform y proxy de colisión original.
`RegionArt` es un componente hijo sin colisión, con escala absoluta 1 y pivote inferior,
referenciado desde el CDO para cook. Su base está 60 cm bajo el origen del marcador regional,
según el contrato actual del materializador. Reubicación vertical/horizontal probada.
Los ids desconocidos conservan el proxy visible. Los materiales son compartidos por asset.

Las colisiones siguen siendo cajas y no reproducen los huecos de la señal. Los cuatro
recursos de seed comparten una forma Q1 sin afirmar composición científica idéntica.
Las vetas comparten contorno exterior; el núcleo se aprecia poco a altura de ojos. Las
capturas muestran sombras negras y texto con poco contraste por la iluminación existente.
Hace falta revisar estos aspectos con una persona antes de ascender a Q2. No hay medición
de rendimiento ni aprobación humana nueva; una captura 1080p no certifica 60 FPS.

## Reproducir

Desde la raíz, con Blender 5.2.1 LTS y Unreal 5.7.4 existentes:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/Blender/generators/region_blockout.py
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -run=pythonscript -script='C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\PrepareRegionBlockoutAssets.py' -unattended -nullrhi -nosplash -nop4 -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\RegionArtImport.log'
& 'C:\Program Files\Blender Foundation\Blender 5.2\5.2\python\bin\python.exe' Tools/Blender/build_region_manifest.py
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' AstraeonEditor Win64 Development 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -WaitMutex -NoHotReloadFromIDE
.\Scripts\RunRegionArtChecks.ps1 -Check Automation
.\Scripts\RunRegionArtChecks.ps1 -Check Visual
.\Scripts\RunRegionArtChecks.ps1 -Check Critical
.\Scripts\RunRegionArtChecks.ps1 -Check Package
.\Scripts\RunRegionArtChecks.ps1 -Check PackagedVisual
.\Scripts\RunRegionArtChecks.ps1 -Check PackagedCritical
```

Generación: 7 repeticiones deterministas, 7 controles negativos de escala y 1 control de
geometrías distintas; 7 validaciones de dimensiones/pivotes/topología/materiales/presupuesto
y 7 roundtrips FBX. Importador comprueba hash de configuración/generador/FBX, escala y pivote
antes de guardar. Rechaza sobrescribir contenido cuyo hash de procedencia difiera.
FBX puede cambiar bytes de metadatos entre generaciones; la huella geométrica es la prueba
de determinismo. Una regeneración que cambie hash exige inspección/reimportación explícita,
no borrar el destino ni eludir esa comprobación. Fuentes generadas y previews quedan en
`ContentPipeline/Generated/RegionBlockout/`, ignorado por Git; `.uasset` usa Git LFS.

El smoke de arte crea una partida de prueba seed 13579, acerca al jugador a cada nodo,
comprueba un trace real, envía click izquierdo y `E` a través del controlador y verifica
bitácora/cantidades/rechazo por herramienta. No guarda esa sesión ni toca el slot del jugador.
Capturas editor: `Saved/Screenshots/RegionArt/`; packaged: bajo `Saved` del paquete.
El smoke crítico utiliza su slot temporal existente y comprueba resolución y save/load.
El runner exige código 0, sentinel de éxito y ausencia de tests fallidos/fatales.

## Siguiente bloque

`SKEL_Humanoid_A` y manos FP Q1; validar jerarquía, skinning, flexión de dedos, exportación
y sockets antes de animar escáner/cortadora/taladro. No reabrir el kit de Ítaca ya creado ni
confundir las prioridades de arte con autorización para ampliar generación planetaria.
