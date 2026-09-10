# ADR 0005 — `Terrain.SurfaceContract` vuelve en la Fase 3, no en la Fase 1

## Estado

Aceptada por directiva del propietario el 2026-09-10.

Modifica un criterio de puerta fijado por
[PLAN_TRANSICION_EJECUCION.md](../PLAN_TRANSICION_EJECUCION.md) §3.1 y §4. No modifica ningún
otro punto de esa puerta, ni el principio de `AGENTS.md` §7 de que ninguna prueba en cuarentena
se borra ni se relaja.

## Contexto

La tabla de cuarentena asigna a la Fase 1 el retorno de dos pruebas:
`Astraeon.WorldGen.Terrain.Relief` y `Astraeon.WorldGen.Terrain.SurfaceContract`. La premisa
declarada en §2.2 del plan era que migrarlas costaba *"cambiar la firma y el dominio del ruido,
no la lógica"*.

Al medir qué exige cada una contra lo que el dominio radial ofrecía, esa premisa resultó cierta
para una y falsa para la otra.

**`Terrain.Relief` valida propiedades del terreno**: determinismo por seed, variación entre
seeds, relieve real, que el suelo no excave por debajo del datum, el límite de escalón caminable,
que las montañas se levanten de verdad, y que un claro nivele contra la altura local. Todas son
propiedades de la superficie y todas tienen contraparte radial exacta. Lo único que cambia es que
la altura se consulta por dirección planetaria global en vez de por `(x, y)`.

**`Terrain.SurfaceContract` valida, en su mayor parte, contenido de región**: la plataforma bajo
Ítaca (`ItacaPadHeightCm`), los claros de suelo, las exclusiones de montaña, el rechazo de
coordenadas fuera del campo regional de 320 m, y la superficie autoral de Region A con su zona de
aterrizaje, su cresta de la señal y su corredor caminable. Nada de eso es núcleo planetario:

- `RegionId` → región planetaria con dirección, extensión angular y altitud es un entregable
  **de la Fase 3** (§4 del plan).
- Proyectar spawn, recursos, señal y criatura sobre la esfera es **de la Fase 3**.
- El campo regional de radio fijo desaparece con los patches, que son **de la Fase 2**.

Migrarla en la Fase 1 obligaría a inventar ahora una versión provisional del contexto de región
—centro, claros, exclusiones y plataforma expresados como direcciones— que la Fase 3 va a rehacer
al definir la región planetaria de verdad. Sería escribirla dos veces, y la segunda con el
diseño real en la mano.

## Decisión

`Astraeon.WorldGen.Terrain.SurfaceContract` **vuelve en la Fase 3**, junto al resto de las pruebas
de región y de Ítaca que ya tienen esa fase asignada.

`Astraeon.WorldGen.Terrain.Relief` **vuelve en la Fase 1**, migrada al contrato radial conservando
todas sus aserciones.

## Consecuencias

- La puerta de la Fase 1 deja de exigir el retorno de `SurfaceContract`. El resto de sus criterios
  queda intacto.
- La tabla de cuarentena de `PLAN_TRANSICION_EJECUCION.md` §3.1 y la de `PHASE_STATUS.md` mueven
  esa fila de Fase 1 a Fase 3.
- La puerta de la Fase 3 no cambia de naturaleza: sigue siendo *"la tabla queda vacía"*, y ahora
  esa tabla incluye una fila más. El riesgo que el plan §7 nombra —que la cuarentena se vuelva
  permanente— se mitiga igual que antes: es criterio de puerta, no una nota.
- **Lo que esta decisión NO autoriza**: relajar ninguna aserción de `SurfaceContract` cuando
  vuelva. Se mueve la fecha, no el listón.

## Alternativa descartada

Migrarla en la Fase 1 con un contexto de región provisional. Se descartó porque el trabajo se
tiraría entero en la Fase 3 y porque un contexto inventado sin el diseño de la región planetaria
delante tendería a fijar decisiones de diseño por omisión, que es exactamente lo que el plan
evita al diferir los perfiles a Data Assets hasta la Fase 4.

## Evidencia de que `Relief` sí migró sin perder nada

Medido el 2026-09-10 sobre el tier Lab de 10 km, seis seeds y una rejilla de 61 × 61 direcciones:
peor escalón del suelo **7,2 cm** contra un límite de 45, montaña más alta **5.100 cm**. La prueba
incorpora además una aserción nueva —`WorstGroundStepCm > 1.0`— para que el límite de escalón no
pueda aprobarse con un mundo liso, que es el fallo que la separación en dos capas existe para no
cometer.
