# MVP — La primera señal

**Versión:** 1.0  
**Plataforma:** Windows 11 x64  
**Motor:** Unreal Engine 5.7  
**Duración objetivo:** 30–45 minutos  
**Propósito:** validar el ciclo `explorar → medir → comprender → actuar → registrar`

## 1. Resultado esperado

El jugador despierta dentro de Ítaca, consulta una señal, llega a una región planetaria, evalúa si puede sobrevivir, explora, escanea un organismo, obtiene recursos, fabrica una solución y alcanza el origen de la señal. La bitácora registra lo aprendido y la partida puede guardarse y continuarse.

## 2. Recorrido crítico

1. Iniciar nueva partida e ingresar una seed opcional.
2. Despertar en el interior mínimo de Ítaca.
3. Recibir de ARGOS una señal desconocida.
4. Consultar datos orbitales y ambientales preliminares.
5. Descender o realizar una transición controlada a la superficie.
6. Medir gravedad, presión, temperatura y composición respirable.
7. Elegir protección básica adecuada.
8. Explorar terreno generado por seed.
9. Encontrar y escanear un mob.
10. Detectar y recolectar tres recursos.
11. Fabricar una solución necesaria para progresar.
12. Revelar el mapa mediante recorrido o escaneo.
13. Llegar a la fuente de la señal.
14. Registrar el hallazgo en la bitácora.
15. Guardar, cerrar y continuar correctamente.

## 3. Alcance funcional

### 3.1 Primera persona

- Caminar, mirar, saltar y agacharse si aporta al recorrido.
- Interactuar mediante una interfaz común.
- Usar escáner y herramienta de recolección.
- Feedback de interacción y estado.

### 3.2 Ítaca

- Una estancia pequeña explorable.
- Consola de misión.
- Consola o acceso a la bitácora.
- Elementos narrativos: títulos, notas y señales del origen humano.
- Transición a la región planetaria.

No requiere nave pilotable ni interior completo.

### 3.3 Región procedural

- Seed visible y configurable al crear partida.
- Relieve o composición espacial determinista.
- Zonas de aparición de recursos.
- Posición de señal o ruina.
- Puntos de interés menores.
- Spawn de criatura.
- Condiciones ambientales derivadas de parámetros válidos.

La región puede ser acotada. No requiere planeta esférico completo.

### 3.4 Ciencia ambiental

Datos mínimos:

- `GravityMS2`
- `TemperatureKelvin`
- `PressureKPa`
- Fracciones principales de atmósfera.
- Indicador de respirabilidad.
- Riesgo ambiental compuesto.

Las consecuencias deben ser jugables:

- Consumo de oxígeno.
- Alteración de movilidad por gravedad.
- Daño o advertencia por temperatura/presión.
- Decisión sobre protección.

### 3.5 Escáner

Debe identificar:

- Ambiente.
- Recursos.
- Mob.
- Señal o estructura.

Estados mínimos:

- Desconocido.
- Escaneo superficial.
- Confirmado.

### 3.6 Mob

- Una especie y una variante visual como mínimo.
- Patrulla o desplazamiento.
- Percepción del jugador.
- Reacción de alerta.
- Ataque o amenaza evitable.
- Pérdida de interés.
- Datos escaneables.

No requiere ecosistema completo, reproducción ni domesticación.

### 3.7 Recursos y crafting

- Tres recursos recolectables.
- Uno debe ser característico de la seed o región.
- Inventario simple.
- Una receta obligatoria para avanzar.
- Al menos una propiedad científica relevante para justificar la receta.

Ejemplo aceptable: fabricar un aislante o filtro con materiales cuyas propiedades se presentan mediante el escáner.

### 3.8 Bitácora y mapa

- Crear entradas automáticamente a partir de evidencia.
- Consultar ambiente, mob, recursos y señal.
- Mostrar nivel de certeza.
- Persistir descubrimientos.
- Revelar mapa por exploración o escaneo.

### 3.9 Narrativa mínima

- Introducción breve en Ítaca.
- Objetivo claro.
- Una señal misteriosa.
- Descubrimiento final que abra preguntas sin implementar una civilización completa.

### 3.10 Guardado

- Seed y versión del generador.
- Transform del jugador.
- Estado ambiental relevante.
- Inventario.
- Descubrimientos.
- Mapa revelado.
- Objetivo actual.
- Recursos recolectados y deltas esenciales.

## 4. Requisitos no funcionales

- Single-player offline.
- Proyecto C++ compilable.
- Lógica esencial verificable sin editar Blueprints complejos.
- Build Development para Windows.
- Sin plugins pagos.
- Sin dependencia de servicios web durante gameplay.
- Sin errores críticos ni crashes conocidos en el recorrido principal.
- Determinismo probado.
- Datos y fórmulas con unidades documentadas.

## 5. Criterios de aceptación

### AC-01 — Arranque

**Dado** un build limpio, **cuando** se inicia, **entonces** aparece el menú y puede comenzarse una partida sin errores críticos.

### AC-02 — Seed

Dos partidas nuevas con la misma seed producen los mismos parámetros ambientales, distribución esencial y objetivo. Seeds diferentes producen diferencias jugables medibles.

### AC-03 — Ítaca

El jugador puede moverse, consultar la misión y comenzar la expedición.

### AC-04 — Ambiente

El escáner informa gravedad, temperatura, presión y respirabilidad; al menos dos de estas variables producen consecuencias observables.

### AC-05 — Exploración

El terreno permite completar el recorrido sin quedar atrapado y posee una ruta alternativa o decisión de navegación.

### AC-06 — Mob

La criatura detecta y reacciona al jugador, puede evitarse y puede escanearse sin necesidad obligatoria de matarla.

### AC-07 — Recursos

Los tres recursos pueden detectarse, recolectarse y persistir correctamente.

### AC-08 — Crafting

La receta consume insumos, produce el objeto correcto y permite superar una barrera del recorrido.

### AC-09 — Bitácora

Cada descubrimiento obligatorio crea o mejora una entrada y conserva su nivel de certeza.

### AC-10 — Mapa

El mapa comienza incompleto y revela zonas de forma persistente.

### AC-11 — Guardado

Después de guardar, cerrar y abrir, el jugador continúa con seed, inventario, descubrimientos y progreso correctos.

### AC-12 — Final

La señal puede alcanzarse, el hallazgo queda registrado y aparece una finalización clara del vertical slice.

### AC-13 — Calidad técnica

Compilación limpia del código del proyecto, pruebas obligatorias aprobadas y ausencia de crashes durante tres recorridos consecutivos.

### AC-14 — Rendimiento

En el hardware de referencia, la experiencia mantiene un promedio objetivo de 60 FPS en 1080p con ajustes razonables y no presenta bloqueos sostenidos durante generación.

## 6. Hitos

### H0 — Bootstrap reproducible

- Repositorio configurado.
- Proyecto C++ creado.
- Build del editor correcta.
- Automatización y documentos vivos creados.

### H1 — Caminata vertical

- Primera persona.
- Mapa de prueba.
- Seed.
- Una medición.
- Una entrada de bitácora.
- Guardado mínimo.

### H2 — Mundo y supervivencia

- Región procedural.
- Ambiente completo del MVP.
- Consecuencias del traje.
- Mapa revelable.

### H3 — Vida y conocimiento

- Mob.
- Escaneo progresivo.
- Bitácora integrada.

### H4 — Recursos y solución

- Recolección.
- Inventario.
- Crafting.
- Barrera superable mediante conocimiento.

### H5 — La primera señal

- Ítaca.
- Flujo narrativo.
- Objetivo final.
- Recorrido completo.

### H6 — Candidato MVP

- Guardado robusto.
- Pruebas.
- Rendimiento.
- Empaquetado.
- Informe final.

## 7. Pruebas obligatorias

- Seed idéntica.
- Seeds diferentes.
- Rangos ambientales válidos.
- Clasificación de respirabilidad.
- Consumo de oxígeno.
- Conversión de unidades.
- Inventario y receta.
- Actualización de bitácora.
- Serialización y carga.
- Apertura de mapas.
- Flujo principal.
- Build empaquetada.

## 8. Fuera del MVP

Todo lo siguiente está congelado:

- Sistemas estelares múltiples.
- Vuelo libre y órbitas completas.
- Civilizaciones y NPC sociales.
- Idiomas y traductor avanzado.
- Planetas completos.
- Construcción libre y colonias.
- Farming avanzado.
- Ejércitos e invasiones sistémicas.
- Muerte permanente y sucesores.
- Hijos e híbridos.
- IA generativa durante el juego.
- Multijugador.

## 9. Puerta de salida

El MVP termina con una build para prueba humana, no con nuevas funcionalidades. Después de esa prueba se decide:

- Continuar.
- Corregir el bucle central.
- Reducir sistemas.
- Rehacer una parte.
- Iniciar la fase de mundo vivo.

