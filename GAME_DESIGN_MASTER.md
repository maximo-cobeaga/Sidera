# ASTRAEON: Legado del Vacío

## Documento maestro del juego

**Versión:** 0.3 — sincronizado con el ADR 0004  
**Estado:** visión rectora. El alcance ejecutable inmediato está en `PHASE_STATUS.md`  
**Motor:** Unreal Engine 5.7.4  
**Plataforma inicial:** Windows 11  
**Modo principal:** un jugador, offline  
**Perspectiva:** primera persona; tercera persona opcional sólo en una fase posterior

> **Sincronizado con el [ADR 0004](ADR/0004-planetas-esfericos-fundacionales.md) el 2026-09-09.**
> La visión de este documento **no se recorta**: sigue siendo el destino. Lo que cambió es el
> orden de construcción.
>
> Donde este documento sitúa el sistema estelar, los planetas esféricos y el viaje continuo en
> una fase tardía, el ADR 0004 **adelanta su núcleo técnico** —coordenadas jerárquicas, cuerpo
> esférico cerrado y gravedad radial— por delante del vertical slice. El razonamiento: no es
> contenido, es la geometría sobre la que se apoya todo lo demás, y construir sistemas de
> superficie sobre un plano significa construirlos dos veces.
>
> Lo que **sí** permanece tardío es el *contenido*: civilizaciones, ciudades, historia procedural,
> genética, sucesión y expansión galáctica. La arquitectura debe admitirlos; ninguna fase actual
> los implementa.
>
> Orden vigente: `PLAN_TRANSICION_EJECUCION.md` §4. Esta visión guía contratos; no habilita
> implementación prematura.

---

## 1. High concept

ASTRAEON es una aventura espacial científica de mundo abierto en la que cada partida crea, a partir de una seed, un cosmos persistente con estrellas, planetas, lunas, ecosistemas, especies, civilizaciones, conocimientos e historias propias.

El jugador no aprende ciencia porque el juego lo examine: aprende porque comprender presión, gravedad, química, órbitas, energía, biología y probabilidades le permite sobrevivir, explorar mejor y tomar decisiones que cambian el mundo.

El juego combina:

- Exploración y asombro.
- Supervivencia científica.
- Aventura en primera persona.
- Crafting, farming y construcción de bases.
- Mobs y ecosistemas.
- Descubrimiento, arqueología y bitácora.
- Diplomacia, ciudadanía, conquista y legado.
- Estrategia a distancia como consecuencia de la aventura, no como sustituto de ella.

## 2. Promesa al jugador

> Cada mundo tiene causas, memoria y consecuencias. Lo que descubrís, construís, destruís y legás continúa después de vos.

Una partida debe producir historias como:

> “Llegué a un planeta bloqueado por mareas. Sus ciudades se desplazaban dentro de una franja templada. Descubrí que una antigua guerra había alterado la atmósfera, ayudé a una facción a reparar sus colectores y, décadas después para ellos, mi hija híbrida regresó para defender la colonia que fundamos.”

## 3. Pilares de diseño

### 3.1 Explorar

El universo no se entrega completo. El jugador observa, viaja, mide, escanea, pregunta, interpreta y cartografía.

### 3.2 Medir

Los instrumentos son parte del mundo: visor, traje, escáner, laboratorio, telescopio, sondas y sistemas de Ítaca.

### 3.3 Comprender

Los datos se convierten en conocimiento. La comprensión abre rutas, recetas, tecnologías, soluciones diplomáticas y ventajas tácticas.

### 3.4 Actuar

El conocimiento sólo importa si permite elegir: aterrizar o no, respirar o sellar el traje, negociar o infiltrarse, resistir o retirarse.

### 3.5 Registrar

La bitácora conserva evidencia, mapas, especies, recetas, culturas, idiomas, acontecimientos y grados de certeza.

### 3.6 Legar

La muerte del personaje no reinicia el mundo. La historia continúa en otra persona con relaciones, capacidades y problemas heredados.

## 4. No negociables

- La experiencia principal es una aventura completa en primera persona.
- No debe convertirse en un city builder ni en un juego de estrategia abstracta.
- El universo es procedural, pero coherente y determinista por seed.
- La ciencia afecta decisiones reales de gameplay.
- Los mundos no se diferencian sólo por colores.
- Las especies no son ciudadanos aleatorios sin parentesco visual.
- Las civilizaciones tienen historia, conocimiento, conflicto, religión o cosmovisión y formas de legitimidad.
- Encontrar otro ser humano debe ser extraordinariamente improbable.
- El jugador puede morir definitivamente; el mundo continúa.
- El juego debe ser disfrutable sin conexión y sin depender de IA generativa en tiempo real.

## 5. Tono y dirección

La obra mezcla asombro, soledad, belleza, peligro y curiosidad. La ciencia no elimina el misterio: permite formular mejores preguntas.

Evitar:

- Humor constante que destruya el tono.
- Exposición enciclopédica obligatoria.
- Razas enteras moralmente idénticas.
- Culturas humanas copiadas con otro nombre.
- Planetas temáticos de una sola textura.
- Tecnología mágica sin costo, límite ni hipótesis.

## 6. Premisa e historia

### 6.1 El protagonista inicial

El jugador comienza como un ser humano excepcional: ingeniero, científico, autodidacta y preparacionista. Anticipó una catástrofe terrestre y construyó en secreto la nave **Ítaca**.

No es un elegido sobrenatural. Sobrevive por preparación, inteligencia, obsesión y una combinación improbable de aciertos.

En la nave conserva títulos universitarios, acreditaciones, fotografías, notas y objetos terrestres. Estos elementos decoran Ítaca, humanizan al personaje y pueden revelar contradicciones sobre su pasado.

### 6.2 Ítaca

Ítaca es hogar, taller, laboratorio, vehículo y archivo. Debe sentirse usada, reparada y construida por una sola persona con recursos limitados.

Funciones futuras:

- Cabina y navegación.
- Taller y fabricación.
- Laboratorio y escáner profundo.
- Invernadero.
- Archivo de conocimiento.
- Módulo médico y genético.
- Bahía de sondas y vehículos.
- Habitaciones y memoriales de tripulantes.

### 6.3 ARGOS

ARGOS es el sistema inteligente de Ítaca. Ayuda a interpretar datos, traducir idiomas y reconstruir modelos, pero no posee conocimiento infalible.

Puede indicar:

- Nivel de confianza.
- Hipótesis alternativas.
- Datos faltantes.
- Posibles sesgos culturales o errores de traducción.

### 6.4 Estructura narrativa principal

La historia autoral proporciona dirección; la simulación procedural crea variaciones y consecuencias.

1. **La noche de Ítaca:** escape, pérdida y puesta en marcha.
2. **Despertar bajo otro cielo:** supervivencia y primera señal.
3. **La palabra imposible:** comunicación y evidencia de inteligencias no humanas.
4. **El precio del legado:** sociedades, conflictos y pertenencia.
5. **Los ecos del origen:** rastros improbables vinculados a la humanidad.
6. **La decisión de Sidera:** elección sobre qué preservar, compartir o transformar.

## 7. Canon y narrativa procedural

El canon se divide en tres capas:

- **Canon fijo:** protagonista inicial, Ítaca, ARGOS, reglas fundamentales y misterio principal.
- **Canon generado:** sistemas estelares, especies, civilizaciones, historias regionales, recursos y conflictos.
- **Canon emergente:** acciones del jugador, muertes, alianzas, colonias, guerras y descendencia.

Regla de coherencia:

> Todo resultado importante debe tener causas previas y consecuencias observables.

Una civilización no odia a otra sólo porque una tabla eligió “hostil”. Debe existir una combinación de memoria histórica, recursos, territorio, ideología, biología, propaganda, instituciones y acontecimientos recientes.

## 8. Generación del universo

### 8.1 Cadena causal

La generación ocurre de mayor a menor escala:

1. Seed raíz de partida.
2. Región galáctica.
3. Sistema estelar.
4. Estrellas y dinámica orbital.
5. Formación de cuerpos.
6. Historia geológica y atmosférica.
7. Clima y biomas.
8. Biosfera y recursos.
9. Especies inteligentes.
10. Civilizaciones e historia.
11. Estado actual, crisis y misterios.
12. Individuos y acontecimientos cercanos.

### 8.2 Jerarquía de seeds

Cada nivel deriva una seed estable de la anterior:

```text
WorldSeed
└── SectorSeed
    └── SystemSeed
        ├── StarSeed
        └── BodySeed
            ├── RegionSeed
            ├── BiosphereSeed
            └── CivilizationSeed
                └── EntitySeed
```

La misma seed y versión del generador deben reconstruir el mismo estado inicial.

### 8.3 Generación diferida

No se simula toda la galaxia al máximo detalle. Cada zona utiliza niveles:

- Resumen estadístico para regiones lejanas.
- Eventos agregados para sistemas conocidos pero no visitados.
- Simulación intermedia para colonias vinculadas al jugador.
- Simulación detallada alrededor del jugador.

Al aproximarse, los datos resumidos se expanden conservando sus resultados anteriores.

## 9. Identidad de estrellas, planetas y lunas

### 9.1 Formación antes que decoración

Cada cuerpo posee un `CelestialBodyGenome` con:

- Seed y versión del generador.
- Tipo, masa, radio, densidad y gravedad.
- Órbita, rotación, inclinación y mareas.
- Edad e historia de formación.
- Núcleo, tectónica, vulcanismo y campo magnético.
- Atmósfera, presión, temperatura y química.
- Agua u otros solventes.
- Clima, ciclos y biomas.
- Minerales y anomalías.
- Biosfera e historia de extinciones.
- Huellas de civilizaciones.
- Rasgos identitarios.

### 9.2 Firma planetaria

Cada mundo relevante recibe entre uno y tres rasgos identitarios fuertes. Un rasgo debe repercutir en al menos tres áreas entre:

- Geografía.
- Supervivencia.
- Exploración.
- Recursos.
- Mobs.
- Arquitectura.
- Tecnología.
- Cultura.
- Religión.
- Historia.

Ejemplo: una tormenta ecuatorial permanente no es sólo un efecto visual; altera sensores, rutas, recolección de energía, migraciones, viviendas y mitología.

### 9.3 Biografía planetaria

Los mundos tienen acontecimientos pasados:

- Impactos.
- Pérdida de océanos.
- Glaciaciones.
- Cambios orbitales.
- Extinciones.
- Terraformación.
- Guerra orbital.
- Colapso ecológico.
- Destrucción o captura de lunas.

Las cicatrices resultantes deben ser observables y utilizables por el gameplay.

### 9.4 Presupuesto de singularidad

Para evitar repetición, el generador asigna:

- Una silueta orbital reconocible.
- Una paleta derivada de composición e iluminación.
- Un fenómeno ambiental dominante.
- Un recurso o proceso geológico significativo.
- Una relación ecológica memorable.
- Una huella histórica o misterio opcional.

No todos los planetas poseen civilizaciones ni anomalías extraordinarias. La rareza conserva el asombro.

## 10. Biosferas y mobs

### 10.1 Generación ecológica

La vida se genera según energía disponible, solvente, atmósfera, gravedad, temperatura, radiación, presión y estabilidad histórica.

Las criaturas deben ocupar nichos:

- Productores.
- Herbívoros o recolectores.
- Depredadores.
- Carroñeros.
- Parásitos y simbiontes.
- Organismos coloniales.
- Vida subterránea, aérea o acuática.

### 10.2 Comportamiento

Los mobs pueden:

- Alimentarse y descansar.
- Migrar.
- Defender territorio o crías.
- Cazar en grupo.
- Reaccionar a luz, sonido, olor, electricidad o campos magnéticos.
- Adaptarse a cambios persistentes.

No toda criatura es enemiga. El desconocimiento puede ser más peligroso que la agresividad.

### 10.3 Escaneo

Niveles posibles:

1. Observación visual.
2. Escaneo superficial.
3. Muestras ambientales.
4. Escaneo profundo en laboratorio.
5. Investigación longitudinal.

La bitácora diferencia hechos, inferencias, relatos e hipótesis.

## 11. Especies y ciudadanos procedurales

### 11.1 Separación fundamental

- **Especie:** biología, anatomía, sentidos, reproducción y adaptaciones.
- **Civilización:** vestimenta, valores, tecnología, símbolos, arquitectura y organización.
- **Individuo:** edad, genética, historia, profesión, relaciones y decisiones.

### 11.2 Genoma de especie

`SpeciesGenome` contiene, entre otros:

- Familia corporal compatible con un esqueleto preparado.
- Simetría y postura.
- Rangos de altura, masa y proporciones.
- Cabeza, ojos, boca y órganos sensoriales.
- Piel, exoesqueleto, pelaje, escamas u otros tejidos.
- Pigmentación y materiales.
- Apéndices compatibles.
- Dimorfismo o polimorfismo.
- Metabolismo, respiración y tolerancias.
- Rasgos hereditarios.
- Capacidades perceptivas y comunicativas.

### 11.3 Construcción gráfica

El juego no genera arbitrariamente un modelo 3D completo mediante IA. Combina familias anatómicas creadas previamente:

- Esqueletos y animaciones compatibles.
- Mallas modulares.
- Cabezas y rasgos.
- Morph targets dentro de rangos seguros.
- Materiales paramétricos.
- Apéndices opcionales.
- Ropa y equipamiento compatibles.

Cada especie establece distribuciones correlacionadas; cada ciudadano hereda una combinación dentro de ellas. Esto crea parecido familiar sin producir clones.

### 11.4 Herencia e híbridos

El sistema futuro utiliza genotipos simplificados y niveles de compatibilidad:

- Compatibilidad natural.
- Compatibilidad mediante tecnología.
- Incompatibilidad biológica.

Los híbridos combinan rasgos validados, no mezclan mallas arbitrariamente. Deben respetarse animaciones, colisiones, ropa y reproducción.

## 12. Civilizaciones

### 12.1 Niveles sociales

- Especie.
- Civilización.
- Estado, reino, comunidad o red política.
- Asentamiento, facción y grupo local.

Una especie puede contener civilizaciones enemigas; una civilización puede integrar varias especies.

### 12.2 Genoma cultural

Cada civilización combina:

- Origen y migraciones.
- Medio ambiente.
- Necesidades biológicas.
- Valores y tabúes.
- Distribución del poder.
- Economía y recursos críticos.
- Tecnología y conocimientos.
- Estética y arquitectura.
- Religión, filosofía o cosmovisión.
- Memoria de conflictos.
- Mecanismo de ciudadanía.
- Mecanismo de legitimidad.
- Actitud hacia otras especies y la IA.

### 12.3 Ecos humanos sin copia

Algunas culturas pueden mostrar destellos reconocibles —monumentalidad piramidal, escritura pictográfica, archivos orales, astronomía ritual, ciudades fluviales— pero nunca copiar una civilización humana completa.

Un eco debe transformarse por:

- Biología alienígena.
- Ambiente planetario.
- Materiales disponibles.
- Historia propia.
- Tecnología.
- Organización social.

### 12.4 Fuentes de conocimiento

El conocimiento puede preservarse mediante:

- Libros o códices.
- Relieves y pictogramas.
- Cantos y tradición oral.
- Profetas o guardianes vivos.
- Memorias biológicas.
- Redes telepáticas.
- Simulaciones.
- Ruinas, máquinas o mapas.

Acceder puede requerir ciudadanía, confianza, intercambio, permiso, infiltración o conquista. Obtener el objeto no garantiza comprenderlo.

### 12.5 Estatus y legitimidad

Cada sociedad define sus rutas de ascenso. Ejemplos procedurales:

- Servicio público.
- Conocimiento demostrado.
- Adopción ritual.
- Elección.
- Linaje.
- Prueba física.
- Duelo formal.
- Acumulación de recursos.
- Reconocimiento religioso.

Desafiar al gobernante sólo permite tomar el trono si las instituciones de esa cultura reconocen esa regla.

### 12.6 Convivencia

La probabilidad de convivencia considera:

- Historia compartida.
- Recursos y territorio.
- Compatibilidad ambiental.
- Valores y tabúes.
- Instituciones.
- Propaganda.
- Interdependencia económica.
- Amenazas comunes.
- Acciones del jugador.

No se reduce a una barra de amistad universal.

## 13. Idiomas y traducción

Modalidades:

- Voz.
- Escritura fonética.
- Pictogramas.
- Color, luz o postura.
- Vibración.
- Química.
- Telepatía.

ARGOS construye modelos de traducción mediante muestras, contexto, objetos, repeticiones y cooperación local.

La traducción progresa desde patrones básicos hasta conceptos culturales. Debe conservar incertidumbre, dobles sentidos y términos intraducibles.

La telepatía no equivale a español mental: puede transmitir imágenes, emociones, relaciones o memorias sin gramática humana.

## 14. Bitácora y mapa

La bitácora es memoria del jugador y sistema de progresión intelectual.

Registra:

- Sistemas, estrellas, planetas y lunas.
- Atmósferas, gravedad, clima y geología.
- Recursos y recetas.
- Especies y mobs.
- Individuos y relaciones.
- Civilizaciones, facciones e historia.
- Idiomas y traducciones.
- Religiones y fuentes de conocimiento.
- Tecnologías.
- Misiones, señales, hipótesis y contradicciones.

Cada afirmación tiene nivel de certeza:

- Observado.
- Medido.
- Inferido.
- Relatado.
- Disputado.
- Confirmado.

El mapa comienza vacío y se construye mediante observación, escaneo, cartografía, compra, intercambio o acceso a archivos ajenos.

## 15. Ciencia integrada

### 15.1 Física

- Gravedad y movilidad.
- Órbitas y ventanas de transferencia.
- Energía y potencia.
- Presión y descompresión.
- Radiación.
- Temperatura y transferencia térmica.
- Dilatación temporal por velocidad y gravedad cuando la escala lo justifique.

### 15.2 Química

- Composición atmosférica.
- Toxicidad y corrosión.
- Aleaciones.
- Combustibles y oxidantes.
- Reacciones para fabricación y soporte vital.
- Solubilidad y separación.

### 15.3 Matemática

- Estimación de riesgos.
- Proporciones y concentraciones.
- Navegación y triangulación.
- Probabilidades.
- Optimización de masa, energía y tiempo.
- Interpretación de señales y patrones.

### 15.4 Tres profundidades

- Consecuencia inmediata para cualquier jugador.
- Explicación breve opcional.
- Modelo detallado en bitácora para quien quiera profundizar.

## 16. Tiempo y relatividad

La partida comienza en una fecha futura procedimental; no en el nacimiento del universo.

El sistema conserva:

- Tiempo coordinado de referencia.
- Tiempo propio del personaje y la nave.
- Calendarios locales.
- Latencia de comunicación.
- Duración de viajes.
- Escalas de simulación remota.

Los efectos relativistas sólo se muestran cuando son materialmente significativos. No se falsean constantemente para parecer científicos.

Consecuencias:

- Colonias envejecen durante viajes.
- Gobiernos y relaciones pueden cambiar.
- Mensajes llegan tarde.
- Un sucesor puede recibir un mundo distinto al que conocía su antecesor.

## 17. Exploración y viaje

La primera persona mantiene presencia física: casco, manos, herramientas, vehículos y cabina.

Escalas:

- Interior y superficie detallada.
- Región planetaria.
- Órbita y sistema.
- Viaje interestelar.
- Simulación distante.

Las transiciones pueden ocultar streaming y cambios de escala sin romper continuidad perceptiva.

## 18. Supervivencia, crafting, farming y bases

### 18.1 Supervivencia

Variables importantes:

- Oxígeno o mezcla respirable.
- Presión.
- Temperatura.
- Radiación.
- Energía.
- Integridad del traje.
- Nutrición y descanso en profundidad moderada.

### 18.2 Crafting por conocimiento

Las recetas se descubren mediante análisis, experimentación, planos, enseñanza o tecnología ajena. Algunos componentes admiten sustitutos según propiedades físicas o químicas.

### 18.3 Farming

Cultivar exige compatibilidad con gravedad, luz, suelo, solventes, atmósfera y biología. Puede utilizar organismos locales o entornos controlados.

### 18.4 Bases

Las bases apoyan la aventura:

- Refugio.
- Laboratorio.
- Extracción.
- Agricultura.
- Comunicaciones.
- Defensa.

No se convierten en el centro exclusivo del juego.

## 19. Combate

Armas y defensas dependen de materiales, atmósfera, gravedad y tecnología. El combate recompensa preparación, lectura del entorno y conocimiento del enemigo.

Alternativas:

- Evitar.
- Ahuyentar.
- Inmovilizar.
- Negociar.
- Sabotear.
- Modificar el entorno.
- Usar armas letales como una opción con consecuencias.

## 20. Colonias, estrategia e invasiones

El jugador puede fundar y habitar colonias, reclutar defensores y delegar autoridad, pero continúa experimentando el mundo mediante acciones físicas en primera persona.

Si una colonia distante es atacada, la información llega con latencia. El jugador puede ordenar:

- Contraatacar.
- Resistir.
- Evacuar.
- Rendirse.
- Activar un plan de contingencia.
- Esperar refuerzos.

Cada opción consume tiempo, recursos, legitimidad o vidas. No existe pausa universal que congele guerras lejanas.

## 21. Muerte, sucesión y genética

La muerte de un personaje es definitiva. El jugador continúa como:

- Hijo o hija.
- Descendiente híbrido compatible.
- Segundo al mando designado.
- Ciudadano elegido dentro de reglas futuras específicas.

El sucesor hereda mundo, relaciones, consecuencias y parte del conocimiento, pero no necesariamente habilidades, ciudadanía, recuerdos privados o legitimidad.

El legado se prepara antes de morir mediante educación, testamentos, archivos, cargos, entrenamiento y vínculos.

## 22. IA generativa

La IA puede asistir durante la creación de contenido estructurado:

- Nombres.
- Variantes históricas.
- Mitos.
- Descripciones.
- Textos de bitácora.
- Eventos y conflictos.

Los algoritmos deterministas y validadores deciden qué puede existir. La IA nunca controla directamente estadísticas críticas, assets incompatibles ni leyes del mundo.

El juego final debe funcionar offline mediante reglas, plantillas y datos preproducidos. La IA online será opcional, no requisito para cargar una partida.

## 23. Dirección artística y audio

- Realismo legible, no hiperrealismo inalcanzable.
- Mundos bellos pero físicamente justificados.
- Interfaz diegética en visor, instrumentos y superficies.
- Siluetas claras para especies, recursos y peligros.
- Iluminación condicionada por la estrella y atmósfera.
- Paisajes sonoros que comuniquen presión, clima, fauna y tecnología.
- El silencio espacial se respeta; vibraciones internas pueden transmitir acciones de la nave.

## 24. Arquitectura técnica objetivo

Módulos conceptuales:

- `AstraeonCore`
- `AstraeonWorldGen`
- `AstraeonScience`
- `AstraeonExploration`
- `AstraeonCreatures`
- `AstraeonKnowledge`
- `AstraeonPersistence`
- `AstraeonNarrative`
- `AstraeonCivilizations` — posterior al MVP
- `AstraeonLegacy` — posterior al MVP

Principios:

- C++ como lógica verificable.
- Blueprints delgados.
- Datos separados de comportamiento.
- Generadores versionados.
- Simulación por niveles de detalle.
- Guardado basado en seed más deltas.
- Interfaces entre sistemas.
- Pruebas de determinismo e invariantes.

## 25. Hoja de ruta macro

### Fase 0 — Fundamentos

Entorno, repositorio, compilación, pruebas y automatización.

### Fase 1 — MVP: La primera señal

Región procedural, ciencia ambiental, escaneo, mob, recursos, crafting, bitácora y guardado.

### Fase 2 — Mundo vivo

Biomas más profundos, cadenas ecológicas, clima y construcción básica.

### Fase 3 — Sistema estelar

Órbita, múltiples cuerpos, navegación y generación astronómica.

### Fase 4 — Primer contacto

Especie, ciudadanos, idioma, traducción, conocimiento y una civilización.

### Fase 5 — Sociedad persistente

Facciones, estatus, ciudadanía, diplomacia, conflicto e historia procedural.

### Fase 6 — Colonias y guerra

Delegación, invasiones, órdenes remotas, logística y simulación distante.

### Fase 7 — Legado

Muerte permanente, sucesión, herencia y genética híbrida.

### Fase 8 — Expansión galáctica

Streaming de sistemas, variedad de familias corporales y contenido raro.

## 26. Métricas de identidad procedural

Antes de aceptar un generador, crear al menos diez seeds y comprobar:

- Cada resultado puede describirse con una frase memorable.
- Las diferencias afectan decisiones, no sólo apariencia.
- Las causas son reconstruibles desde los datos generados.
- No aparecen combinaciones físicamente incompatibles sin explicación.
- La distribución contiene resultados comunes, poco comunes y raros.
- La misma seed reproduce los mismos rasgos esenciales.

## 27. Regla final de producción

La visión completa guía la arquitectura, pero no autoriza implementar todo a la vez.

> Primero demostrar que explorar, medir, comprender, actuar y registrar es divertido. Después ampliar el universo.

