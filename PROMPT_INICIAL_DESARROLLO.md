# Prompt inicial de desarrollo autónomo

## Uso

Este texto se entrega una sola vez al agente, ejecutándolo desde la raíz del repositorio. `AGENTS.md` contiene las reglas persistentes; este prompt inicia el trabajo.

---

Comienza el desarrollo autónomo del MVP **La primera señal** de **ASTRAEON: Legado del Vacío**.

Tu objetivo no es producir una demostración aislada ni una gran cantidad de código: debes entregar un vertical slice jugable, verificable y empaquetado para Windows 11, conforme a los documentos del repositorio.

Antes de modificar archivos:

1. Lee completamente `AGENTS.md`.
2. Lee completamente `Docs/GAME_DESIGN_MASTER.md`.
3. Lee completamente `Docs/MVP_LA_PRIMERA_SENAL.md`.
4. Lee `Docs/WINDOWS_11_SETUP.md` y verifica el entorno disponible.
5. Inspecciona el repositorio y preserva cualquier trabajo existente.

Después:

1. Realiza un diagnóstico no destructivo del entorno:
   - versión de Windows;
   - ruta y versión de Unreal Engine;
   - Visual Studio, MSVC y Windows SDK;
   - Git y Git LFS;
   - espacio disponible;
   - capacidad de invocar UnrealBuildTool y AutomationTool.
2. Si existe un proyecto Unreal, audítalo antes de cambiarlo. Si no existe, crea un proyecto C++ denominado `Astraeon` compatible con Unreal Engine 5.7.
3. Inicializa Git y Git LFS sólo si aún no están configurados.
4. Crea o completa:
   - `.gitignore` adecuado para Unreal;
   - `.gitattributes` para Git LFS;
   - `Docs/BACKLOG.md`;
   - `Docs/DEVELOPMENT_STATE.md`;
   - `Docs/DECISIONS.md`;
   - `Docs/TEST_REPORT.md`;
   - `Docs/KNOWN_ISSUES.md`;
   - `Docs/ADR/`.
5. Descompón el MVP en hitos pequeños con dependencias y criterios verificables.
6. Implementa primero una caminata vertical mínima:
   - el proyecto compila;
   - abre un mapa de prueba;
   - permite comenzar una partida;
   - genera una región a partir de una seed;
   - muestra una medición ambiental;
   - registra un descubrimiento;
   - guarda y recupera ese estado.
7. Continúa autónomamente con el backlog completo siguiendo el ciclo de `AGENTS.md`.

Reglas de ejecución:

- No pidas confirmación por decisiones menores.
- Elige soluciones simples, reversibles y extensibles; documenta y continúa.
- No implementes funciones excluidas del MVP.
- No uses plugins pagos ni contenido sin licencia verificable.
- No actualices la versión del motor.
- No ocultes fallos ni desactives pruebas para avanzar.
- No realices operaciones destructivas fuera del repositorio.
- No hagas commits cuando el proyecto no compile o fallen las pruebas relevantes.
- Conserva siempre un último estado jugable.

Construye el núcleo en C++. Limita Blueprints a composición visual, parámetros y referencias de assets. Mantén la lógica científica, procedural, de persistencia, interacción y progresión en código comprobable.

Trabaja hasta alcanzar la definición de terminado. Si aparece un bloqueo real, detente únicamente después de documentar la reproducción, los logs, tres estrategias intentadas y la recomendación concreta. Formula como máximo una pregunta que permita desbloquear el desarrollo.

Cuando el MVP esté terminado:

1. Ejecuta todas las pruebas.
2. Genera una build Development de Windows.
3. Realiza un recorrido automatizado o smoke test completo.
4. Actualiza toda la documentación.
5. Etiqueta el estado candidato.
6. Entrega un informe final breve.
7. Detente y solicita una sesión de prueba humana; no amplíes el alcance.

Comienza ahora con el diagnóstico del entorno y continúa sin esperar instrucciones adicionales.

