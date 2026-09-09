# Ítaca — exterior y objetos de la estancia

2026-09-06. La nave deja de ser un cubo gris escalado y la estancia deja de tener cubos por
estaciones. Calidad **Q1**, mismo kit declarativo y mismo contrato estático que el resto de
la familia Ítaca.

## Entrega

Siete assets nuevos en `Content/Astraeon/Art/Blockouts/Itaca/`, junto al kit que ya estaba:

| Asset | Medida real | Uso |
|---|---|---|
| `SM_Itaca_Hull_Blockout` | 13,0 × 7,0 × 4,4 m | Casco: perfil extruido con morro rasante y cola |
| `SM_Itaca_Engine_Blockout` | 3,6 × 1,5 × 1,5 m | Góndola con toma y tobera; se instancia ×2 |
| `SM_Itaca_LandingGear_Blockout` | 0,9 × 0,5 × 0,6 m | Patín con zapata; se instancia ×4 |
| `SM_Itaca_Antenna_Blockout` | 0,7 × 0,7 × 1,1 m | Mástil y array sobre el lomo |
| `SM_Itaca_HatchLeaf_Blockout` | 1,3 × 0,1 × 2,2 m | Hoja de la escotilla, sobre bisagra real |
| `SM_Itaca_PilotConsole_Blockout` | 0,95 × 0,70 × 1,25 m | `itaca_pilot_console` |
| `SM_Itaca_Fabricator_Blockout` | 1,10 × 0,75 × 1,15 m | `itaca_fabricator` |

Reutilizan los materiales planos del kit (`M_Blockout_Hull`, `_Service`, `_Instrument`): son
la misma familia que la habitación a la que pertenecen, no una paleta paralela. Las consolas
comparten el material de instrumento con ARGOS; la mesa de fabricación usa el de servicio,
porque es maquinaria y no un terminal.

## El envolvente, comprobado sobre el montaje

El plan maestro fija un volumen de estudio de 14 × 10 × 6 m. Comprobarlo sobre el casco
suelto no diría nada, así que el generador valida el **conjunto montado** con las posiciones
reales de motores, patines y antena, que viven en el bloque `assembly` del config y se
replican como constantes en `AAstraeonShipPawn`. Resultado: **13,0 × 10,0 × 5,87 m**.

Dos comprobaciones más, que fallan la generación entera si se rompen:

- El casco tiene que **contener la estancia de 8 × 6 × 2,8 m**. Si no, el exterior y el
  interior dejan de ser el mismo objeto de ficción, que es justo lo que decía el placeholder
  anterior: un cubo de 9 × 6 × 2,2 m que era *más bajo* que la habitación que contenía.
- Los patines tienen que **sostener el casco**, no quedar enterrados bajo el vientre.

Ese segundo control salió de mirar la primera preview: el casco apoyaba el vientre en el
suelo y las patas quedaban completamente ocultas debajo. La corrección fue convertirlas en
patines cortos y elevar el casco 0,45 m sobre ellos. Al hacerlo, la antena de 2,2 m se salía
del techo de 6 m, así que bajó a 1,1 m. El contrato ahora impide repetir cualquiera de los dos
errores.

## Integración

**Nave** (`AAstraeonShipPawn`): los módulos se montan como componentes separados —casco, dos
motores, cuatro patines y antena— en vez de un casco monolítico, porque las toberas y los
patines son los anclajes naturales de los VFX de empuje y de polvo de aterrizaje que todavía
no existen. Se guardan en `HullModules` para que ese trabajo no tenga que buscarlos por nombre.

El origen del actor pasó a ser el punto de apoyo de los patines, así que la altitud que traza
el vuelo contra el terreno es directamente la altura a la que aterriza la nave. El brazo de
cámara pasó de 1.400 a 2.600 cm y de 220 a 620 cm de alto: con el anterior, una nave de 13 m
dejaba la cámara dentro del casco.

**Estaciones** (`AAstraeonRegionMarker`): `itaca_pilot_console` e `itaca_fabricator` reciben
el mismo tratamiento que ARGOS —malla apoyada en el suelo a escala 1, y una caja de colisión
que reproduce su silueta en vez de un cubo del motor escalado—. Se añadió `IsItacaStation()`
porque el marcador teñía el material de cualquier marca que no fuese ARGOS, y eso habría
borrado la paleta del kit en las dos estaciones nuevas.

**Escotilla** (`AAstraeonItacaInterior`): la hoja cuelga de una bisagra real en la jamba de
estribor y **empieza cerrada, bloqueando el paso**. Abrir con `E` sobre el marcador ESCOTILLA
la hace girar 95° hacia dentro de la estancia en poco más de un segundo, y el bloqueo se
levanta apenas empieza el giro. Esto resuelve la entrada de `KNOWN_ISSUES.md` que el
propietario había marcado como "ahora o más adelante": *"la ESCOTILLA no bloquea el paso
físicamente… funciona como una puerta siempre abierta"*.

El detalle que hace que siga funcionando: la caja de la hoja **ignora el canal de
visibilidad**. Frena a la cápsula del jugador, pero deja pasar el trazo de interacción, así
que el marcador ESCOTILLA que hay detrás sigue siendo alcanzable. Sin eso, cerrar la puerta
la habría vuelto imposible de abrir.

## Reproducción

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/Blender/generators/ship_blockout.py
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Users\MAXIMO\Desktop\Astraeon\Astraeon.uproject' -run=pythonscript -script='C:\Users\MAXIMO\Desktop\Astraeon\Scripts\Editor\PrepareShipBlockoutAssets.py' -unattended -nullrhi -nosplash -nop4 -abslog='C:\Users\MAXIMO\Desktop\Astraeon\Saved\Logs\ShipPackages.log'
```

El generador es autocontenido por la misma razón que el de la criatura: el sha256 de
`itaca_blockout.py` es entrada de un reporte aprobado, así que se reutilizan sin tocar el
validador estático (`validators/mesh.py`), los controles negativos (`tests/checks.py`) y el
exportador, y este runner vive al lado en vez de extender aquel.

## Verificación

- Blender: 17/17 controles (10 negativos y 7 repeticiones de determinismo), 7 assets validados
  contra el contrato estático completo —pivote al suelo y centrado en XY, manifold, normales
  hacia fuera, recuento de shells, material único, presupuesto de triángulos— y 7 roundtrips FBX.
- Unreal 5.7.4: 7 paquetes guardados con dimensiones exactas en centímetros y pivote al suelo
  comprobado. Exit 0, commandlet 0 errores / 0 warnings.
- `AstraeonEditor Win64 Development`: compilación limpia.
- Automation `Astraeon.*`: **51 encontrados, 51 exitosos**, exit 0.
- Smoke crítico editor-game, que ahora atraviesa una escotilla que empieza cerrada:
  `Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true Seed=13579`.
- Test nuevo `Astraeon.Art.Itaca.ExteriorAndStations`: cuenta un casco, dos motores, cuatro
  patines y una antena sin colisión y a escala 1, comprueba que no sobreviva ningún cubo del
  motor en la nave, que el casco contenga la estancia y entre en el volumen de estudio, que
  las dos estaciones traigan malla propia apoyada en el suelo con caja bloqueante, y que la
  hoja empiece cerrada, bloquee, ignore el canal de visibilidad y deje de bloquear al abrirse.

Evidencia: `ContentPipeline/reports/ship_blockout_validation.json` y `ship_content_packages.json`.
Previews: `ContentPipeline/Generated/ShipBlockout/Itaca_Exterior_Preview.png` y
`Itaca_Interior_Preview.png`.

## Pendiente de revisión humana y límites

- **Revisar las dos previews.** El exterior lee como un carguero de superficie romo apoyado
  en cuatro patines; es deliberadamente utilitario y de caras planas, para no chocar con las
  paredes de cajas de la estancia. Si se quiere algo más esbelto, el perfil del casco es una
  sola lista de puntos en el config.
- **La hoja no cierra sola.** Una vez abierta queda abierta durante la sesión, y el estado no
  se guarda. Cerrarla al volver a entrar es una decisión de diseño, no un bug.
- **El puesto de pilotaje no es una cabina.** La consola existe, pero no hay asiento, ni
  ventana, ni vista al exterior desde dentro: el interior y el exterior siguen siendo dos
  actores que nunca se ven a la vez.
- **Sin animación de despegue ni aterrizaje.** La estancia se oculta y aparece la nave, como
  antes. Los patines no se repliegan; los motores no tienen ciclo. Ambos ahora tienen dónde
  colgarse, pero nada de eso está hecho.
- El hueco de la escotilla en el casco exterior **no está modelado**: la hoja es una pieza del
  interior. Exterior e interior no comparten registro geométrico todavía.
- Materiales planos, UV de prototipo, sin texturas, sin VFX ni audio.
