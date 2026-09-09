# Pipeline Blender → validación → FBX → Unreal

## Extensión de exterior de Ítaca y estaciones — 2026-09-06

Casco, motor, patín, antena, hoja de escotilla, consola de pilotaje y mesa de fabricación:
`configs/ship_blockout.json`, generador `generators/ship_blockout.py`, persistencia
`Scripts/Editor/PrepareShipBlockoutAssets.py`. Guía y límites en
`ITACA_EXTERIOR_INTEGRATION.md`.

Reutiliza **sin modificarlos** el validador estático (`validators/mesh.py`), los controles
negativos (`tests/checks.py`) y el exportador de la familia Ítaca, y usa el mismo formato
declarativo del config: cada asset es una lista de cajas o un perfil `profile_xz` extruido.
El runner vive al lado de `itaca_blockout.py` en vez de extenderlo, porque el sha256 de aquel
generador es entrada de un reporte de validación ya aprobado.

Aporte nuevo del perfil: **validar el conjunto montado, no las piezas sueltas**. El bloque
`assembly` del config lleva las posiciones de motores, patines y antena, y el generador
comprueba con ellas que el montaje entre en el volumen de estudio de 14 × 10 × 6 m, que el
casco contenga la estancia de 8 × 6 × 2,8 m y que los patines sostengan el casco en vez de
quedar enterrados bajo el vientre. Esos mismos valores se replican como constantes en
`AAstraeonShipPawn`; si divergen, el montaje deja de ser el que se validó.


## Extensión de criatura — 2026-09-06

Cuadrúpedo `SKEL_Quadruped_A`, dos mallas y siete clips de estado:
`configs/creature_blockout.json`, generador `generators/creature_blockout.py`, validación
`validators/creature.py`, persistencia `Scripts/Editor/PrepareCreatureBlockoutAssets.py`,
manifiesto `Tools/Blender/build_creature_manifest.py`. Guía y límites en
`CREATURE_ART_INTEGRATION.md`.

El generador es **autocontenido a propósito**: el sha256 de `generators/humanoid_blockout.py`
es entrada de un reporte de validación ya aprobado, así que refactorizar sus helpers a un
módulo común invalidaría esa evidencia y obligaría a repetir todo el lote humano. Se duplican
`tube`, `ellipsoid` y `box` en lugar de tocar un generador congelado. Lo mismo vale para
`validators/creature.py` frente a `validators/humanoid.py`.

Contrato de apoyo para animación de criaturas: cada fotograma se posa, se mide y se apoya en
el suelo, y sólo después se suma el vaivén que el clip declara como `lift`. Ningún clip puede
flotar ni hundirse por accidente y todo movimiento vertical queda declarado. La consecuencia
práctica es que **una pose sólo baja el cuerpo si lo baja de verdad**: no sirve trasladar.


## Extensión de herramientas y persistencia en Content — 2026-09-06

Siete props de mano más la broca del taladro y siete animaciones de uso:
`configs/tools_blockout.json`, generador `generators/tools_blockout.py`, validación
`validators/tools.py`, importación de prueba `Scripts/Editor/ValidateToolsImport.py`,
manifiesto `Tools/Blender/build_tools_manifest.py` →
`ContentPipeline/tools_asset_manifest.json`.

**Contrato de ejes, aprendido a la mala:** al importar con `convert_scene`, Unreal pasa del
marco diestro de Blender al suyo negando Y. Las *dimensiones* no lo notan porque son
absolutas; cualquier comprobación de **origen, centro o pivote fuera del eje Z** sí. Todo
chequeo nuevo que compare posiciones contra las medidas de Blender debe negar Y primero, y
conviene que registre el valor medido y el esperado en el reporte, no sólo el mensaje de
error. Para transforms relativos completos (por ejemplo la pose de agarre en el espacio de un
hueso) la equivalencia es conjugar por C = diag(1,-1,1), y Unreal usa vectores fila, así que
además hay que transponer.

Dos scripts guardan paquetes reales de `Content/`, con el mismo patrón fail-closed que
`PrepareRegionBlockoutAssets.py`:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -run=pythonscript -script='C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\PrepareHumanoidBlockoutAssets.py' -unattended -nullrhi -nosplash -nop4 -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\HumanoidPackages.log'
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -run=pythonscript -script='C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\PrepareToolsBlockoutAssets.py' -unattended -nullrhi -nosplash -nop4 -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\ToolsPackages.log'
```

El primero debe correr antes que el segundo: crea `SKEL_Humanoid_A`, al que se atan las
animaciones de herramienta. Ambos verifican hashes de config/generador/validador y de cada
FBX, hacen preflight de todos los destinos antes de importar o guardar nada, y comparan el
tag `AstraeonSourceHash` de lo que ya exista antes de reimportar.

Los materiales planos se crean por **nombre de slot**, que viaja en el FBX desde el nombre
del material de Blender. Un slot fuera de la paleta del config aborta el script, en vez de
dejar el material gris por defecto del motor.

Integración jugable y límites: `CHARACTER_TOOLS_INTEGRATION.md`.


## Extensión humana — 2026-09-06

Primer rig `SKEL_Humanoid_A`, cuerpo, manos FP y siete clips Q1. Guía y comandos:
`HUMANOID_ART.md`. Generador `generators/humanoid_blockout.py`, configuración
`configs/humanoid_blockout.json`, validación específica `validators/humanoid.py`.
Nueve roundtrips y nueve importaciones Unreal **transitorias** aprobados; no integrados
al gameplay. Este perfil añade controles reales de armature, skin y animación; los
controles estáticos descritos más abajo siguen limitados a sus familias originales.

## Extensión regional — 2026-09-06

Siete blockouts de recursos/señal/anomalía: `configs/region_blockout.json`, generador
`generators/region_blockout.py`, importador `Scripts/Editor/PrepareRegionBlockoutAssets.py`
y manifiesto `ContentPipeline/region_asset_manifest.json`. Reutilizan validación estática y
exportación del kit. Guía y comandos completos en `REGION_ART_INTEGRATION.md`.
El Python estándar del sistema no está instalado; para el manifiesto se verificó el
ejecutable incluido en Blender: `Blender 5.2/5.2/python/bin/python.exe`.

## Entorno y estructura

Blender declarado por el entorno: 5.2.1 LTS, ejecutable `C:\Program Files\Blender Foundation\Blender 5.2\blender.exe`. La versión observada se registra automáticamente en cada reporte; no actualizar Blender, Unreal 5.7.4 ni toolchain como parte de esta tarea.

- `Tools/Blender/common/`: inicialización métrica, materiales planos, geometría y huella semántica.
- `configs/`: dimensiones, presupuestos, materiales y composición textual.
- `generators/`: orquestación de creación/validación/exportación.
- `validators/`: contrato de static meshes blockout.
- `exporters/`: FBX por objeto.
- `tests/`: controles negativos y reproducción de cada asset.
- `ContentPipeline/Generated/ItacaBlockout/`: `.blend`, FBX individuales y preview regenerables, ignorados por Git.
- `ContentPipeline/reports/`: evidencia JSON versionable.
- `ContentPipeline/asset_manifest.json`: catálogo y estado; no equivale a integración en gameplay.

`graphics/` se conserva como prueba previa. Fuentes nuevas son código/config; si se incorporan fuentes manuales `.blend`, Git LFS está configurado. Nunca versionar cachés, logs o builds. Outputs generados permanecen locales.

## Ejecución desde la raíz

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/Blender/generators/itaca_blockout.py
```

Este comando ejecuta pruebas, genera cinco assets, valida, exporta cada uno, guarda fuente canónica con todos los pivotes en cero, reimporta cada FBX para comprobar escala y crea una escena/imagen de presentación separada. La escena fuente contiene los assets superpuestos intencionalmente en origen: aislar un objeto para editarlo. `Itaca_Blockout_Preview.blend` es la escena dispuesta para revisión, no exportarla entera.

La API reutilizable es `validate(obj, spec)` y `export_asset(obj, directory)`. Validar antes de exportar. El runner falla con exit 1 mediante `--python-exit-code 1` y escribe `passed:false`/error en JSON; un FBX previo no prueba que la última ejecución haya pasado. Consultar siempre el reporte y salida actuales.

Importación de prueba en Unreal (paquetes sólo en memoria; no guarda `.uasset` ni mapa):

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -run=pythonscript -script='C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\ValidateArtBlockoutImport.py' -unattended -nullrhi -nosplash -nop4 -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\AstraeonArtImport.log'
```

Comprueba huella del FBX validado e importa con `FbxFactory`, conversión de escena/unidad y escala 1; registra bounds en cm en `itaca_unreal_import.json`. Rechaza reemplazar un destino de prueba existente. Usa cachés locales normales de Unreal; el sandbox puede impedir iniciar Zen. La importación no habilita colisión ni produce materiales Unreal.

Después de generar y probar importación, actualizar manifiesto:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/Blender/build_manifest.py
```

`build_manifest.py` también funciona con Python estándar, pero en esta máquina `python` es un alias de Microsoft Store sin intérprete instalado. Reusar el Python de Blender, sin instalar dependencias. El manifiesto verifica hashes de configuración y FBX, referencias y ciclos de dependencias; un reporte de importación que no coincida con el FBX actual deja esa validación pendiente. Tras regenerar FBX, repetir importación antes de promover su estado.

Controles: nombres, duplicados, mesh, valores finitos, rotación/escala aplicadas, origen, dimensiones, apoyo Z/centro XY, caras degeneradas/ngons, manifold/winding, volumen positivo por shell, UV debug, presupuesto de tris, material único, ausencia de padres/modificadores/texturas externas. El marco tiene una shell cerrada y hueco real. Humano/consola contienen varias piezas rígidas: no certificados para destrucción, skin o retopología final.

Pruebas negativas inyectan escala, nombre, pivote, UV/material ausente, modificador, normales invertidas y hueco. Repetición compara huellas semánticas de geometría por asset. El roundtrip verifica un mesh por FBX y dimensiones métricas. Rigs, texturas de producción y colisión Unreal requieren controles nuevos cuando entren en alcance.

## Contrato de intercambio

Blender métrico, Unit Scale=1, 1 unidad=1 m. Rotación y escala aplicadas; objetos canónicos en origen. FBX Forward=-Y, Up=Z, global scale=1 y conversión de unidades habilitada. Un asset por FBX. No compensar errores con escala 100 al importar.

Unreal Import Uniform Scale=1; comprobar que panel de 2 m mide 200 cm y humano 180 cm. Verificar frente con consola y normales en viewport. Importar a un destino de ensayo antes de conectar runtime; nunca reemplazar mapa bootstrap ni componentes de colisión por el simple hecho de disponer de arte.

Materiales bpy no son materiales maestros Unreal. Los colores planos sirven para blockout; UV0 actual se superpone por cara y sólo sirve para debug. No usarlo como lightmap: generar canal apropiado en Unreal o iluminación dinámica. Props comunes usan convexos automáticos; marco requiere varias cajas de colisión o ensayo estático por triángulos, porque un convexo único puede tapar el hueco.

Planetas: pivote central, reglas completas en `GUIA_ARTE_PLANETAS_BLENDER.md`. Texturas futuras: BaseColor sRGB; Normal/ORM/Height/DetailMask lineales. Triplanar y blending por datos runtime, nunca por splat map individual de planeta.

## Recuperación

- Si falla: leer JSON y log completo, corregir fuente/config y repetir; no desactivar controles.
- Si tamaño resulta ×100/÷100: comprobar units, export y conversión de import; no escalar el actor para ocultarlo.
- Normales/manifold: corregir shell en fuente; no aceptar una malla abierta como destructible.
- Si el render falla después de exportar: reporte global queda fallido; verificar motor CPU y error antes de declarar slice listo.
- Ejecutar siempre en background/factory-startup para no depender del archivo abierto del usuario. El script reemplaza sólo sus outputs derivados, no `graphics/astra_test.blend` ni Content del juego.

El rendimiento y la navegación jugable no se certifican en Blender. Estado de verificación concreto y deuda en `TEST_REPORT.md`, `KNOWN_ISSUES.md` y el manifiesto.
