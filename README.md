# ASTRAEON: Legado del Vacío

Repositorio de desarrollo del videojuego **ASTRAEON: Legado del Vacío**.

ASTRAEON es una aventura espacial científica en primera persona, principalmente offline y para un jugador. Su ciclo central es:

> explorar → medir → comprender → actuar → registrar

## Estado

- Fase actual: preproducción técnica e inicio del MVP.
- Primer entregable: vertical slice **La primera señal**.
- Plataforma inicial: Windows 11.
- Motor fijado para el MVP: Unreal Engine 5.7.
- Arquitectura: C++ como núcleo; Blueprints sólo como capa visual y configuración.
- Desarrollo: agente de IA con iteraciones automatizadas, pruebas y control de alcance.

## Documentos obligatorios

El agente debe leer estos archivos antes de modificar el proyecto:

1. [`AGENTS.md`](AGENTS.md): contrato operativo permanente.
2. [`PROMPT_INICIAL_DESARROLLO.md`](PROMPT_INICIAL_DESARROLLO.md): orden de inicio de la ejecución autónoma.
3. [`Docs/GAME_DESIGN_MASTER.md`](Docs/GAME_DESIGN_MASTER.md): visión, universo, sistemas y límites del juego completo.
4. [`Docs/MVP_LA_PRIMERA_SENAL.md`](Docs/MVP_LA_PRIMERA_SENAL.md): alcance verificable del primer MVP.
5. [`Docs/WINDOWS_11_SETUP.md`](Docs/WINDOWS_11_SETUP.md): requisitos e instalación del entorno.

En cuanto comience el desarrollo, el agente deberá crear y mantener:

- `Docs/BACKLOG.md`
- `Docs/DEVELOPMENT_STATE.md`
- `Docs/DECISIONS.md`
- `Docs/TEST_REPORT.md`
- `Docs/KNOWN_ISSUES.md`
- `Docs/ADR/`

## Regla de alcance

Durante el MVP sólo se incorpora aquello que fortalece el ciclo central. Las civilizaciones avanzadas, la genética, las colonias, las guerras y los viajes interestelares forman parte de la visión completa, pero permanecen congelados hasta aprobar el MVP.

## Definición resumida de éxito

Una persona debe poder iniciar una partida, despertar en Ítaca, llegar a una región planetaria generada por seed, medir su ambiente, explorar, escanear una criatura, recolectar recursos, fabricar una solución, descubrir una señal y ver todo registrado en la bitácora. El recorrido debe poder completarse en 30–45 minutos y distribuirse como ejecutable de Windows.

