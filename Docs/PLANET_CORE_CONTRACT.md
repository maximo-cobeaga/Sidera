# Contrato del núcleo planetario — Fase 1

Estado: implementación de laboratorio en verificación, 2026-09-09. ADR 0004 conserva autoridad.

## Definición y coordenadas

`FAstraeonPlanetDefinition` contiene identidad, radio en cm (`double`), masa kg, gravedad m/s²,
nivel de mar relativo al radio, seeds y versión. Esta API mantiene los centímetros explícitos
del marco existente; ningún float guarda una posición planetaria. Dominio inicial validado:
radios de 10 m a 2500 km. El runtime de prueba deriva masa de radio y gravedad mediante
`M=g*(R/100)^2/G`; sus parámetros no definen Khepri ni el canon del juego.

La dirección superficial unitaria identifica un punto del cuerpo. Cara/UV es una dirección
de representación; en una arista el mismo punto pertenece a dos caras y en una esquina a tres.
Las UV están en [-1,1]. La proyección normaliza el punto del cubo:

| Cara | Punto del cubo |
|---|---|
| +X | (1,U,V) |
| -X | (-1,-U,V) |
| +Y | (-U,1,V) |
| -Y | (U,-1,V) |
| +Z | (U,V,1) |
| -Z | (U,-V,-1) |

Empates al invertir: X, después Y, después Z. No se usa una cara para deducir la altura.
Dirección nula/no finita se rechaza; UV fuera de rango y cara inexistente devuelven NaN;
`DirectionToFaceUv` comunica invalidez con `bIsValid=false`. No hay clamps a otra ubicación.
Los ejes anteriores definen el espacio del cuerpo, no un arriba jugable fijo.

## Superficie reproducible

Versión 2: ruido de valor sobre lattice 3D, hash entero uint64 definido por el proyecto,
canales Terrain/Detail y suavizado quíntico. Longitudes de onda de 100 y 50 m; amplitud
combinada acotada a ±1,8 m. Coordenadas de ruido: dirección * radio / longitud de onda.
Dos caras consultan la misma posición, altura y normal. Las normales usan diferencias
centrales tangentes de 1 cm de distancia física. SeaLevel es un dato; no hay océano renderizado.

La versión 1 experimental calculaba un hash independiente por punto cuantizado, por lo que
era discontinua. No hay saves planetarios persistidos con ese algoritmo. Se rechaza una versión
de superficie desconocida explícitamente. Fixture v2: seed 4242, radio 1.000.000 cm, dirección
(1,0,0) => altura -29,68677262553832 cm. Tolerancia de verificación entre builds: 1e-9 cm.

## Representación y colisión

`AAstraeonPlanetRuntime` construye seis caras a resolución uniforme: 32×32 quads por cara,
6.534 vértices y 12.288 triángulos de render. Cada cara sustrae su origen en double antes
de entregar vértices locales a PMC. Escala y rotación del actor deben ser identidad; centro
trasladable. La normal y el relieve dependen del espacio del cuerpo, no del transform de malla.

Sólo se cocina colisión para las celdas vecinas a la dirección del jugador, incluyendo todas
las caras adyacentes. Usa los mismos triángulos que el render. Al recentrar el componente se
desvincula la base del Character: cambiar el origen de la representación no mueve el suelo.
La resolución angular fija todavía NO da detalle cercano uniforme en todos los radios;
quadtree, LOD, workers y cambios generales de frame son entregables de Fase 2.

La consulta es continua, pero el render de bajo LOD es triangulado: su altura entre vértices
puede diferir de la analítica. El spawn de las pruebas de cardinales usa un trace de la
colisión real. No se certifica precisión de render planetario grande sólo por pasar ese trace.

## Laboratorio y puerta de salida

`TL_11_CubeSphereClosed` usa 200 m para una vuelta a pie. Radios de ingeniería se pasan con
`-AstraeonPlanetRadiusCm=1000000`, `50000000` y `250000000`. Son pruebas distintas:
caminar toda la esfera pequeña y verificar puntos separados en las grandes.

El laboratorio no ofrece guardado/carga v2 ni simula una Ítaca plana. El traje se mantiene en
refugio de prueba. Se deshabilita KillZ global y el rescate regional: cualquier caída defectuosa
debe quedar visible y hacer fallar el smoke. No es todavía el vertical slice jugable.

Pendientes de la puerta: portar íntegramente `Terrain.Relief` y `Terrain.SurfaceContract`,
incluyendo sus garantías de relieve, montañas, claros y apoyo; la altura simple de laboratorio
no sustituye esas garantías. Conservar los asserts originales. También se requieren build
Development empaquetada, evidencia visual, rendimiento y aceptación de locomoción antes del cierre.

Después del núcleo verificado: pulido de las animaciones existentes en Blender mediante
Higgsfield Bridge, con presupuesto autorizado máximo de 10 créditos. No regenerar personaje
ni alterar skeleton para resolver poses. Ningún crédito consumido por este contrato.
