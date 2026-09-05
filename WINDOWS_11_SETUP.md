# Preparación del entorno — Windows 11

## 1. Hardware de referencia

Equipo disponible:

- Intel Core i5-14400F.
- NVIDIA RTX 4060 con 8 GB de VRAM.
- 16 GB de RAM.
- SSD de 1 TB.

Evaluación:

- CPU: adecuada para el MVP.
- GPU: adecuada para Lumen y Nanite moderados.
- RAM: funcional, pero constituye el principal límite.
- Almacenamiento: adecuado con al menos 250 GB libres.

Recomendación prioritaria: ampliar a 32 GB de RAM. Mientras se mantengan 16 GB, usar archivo de paginación administrado por Windows, cerrar aplicaciones no necesarias y evitar compilar shaders junto con tareas pesadas externas.

## 2. Sistema base

- Windows 11 x64 actualizado.
- DirectX 12.
- Modo Juego activado.
- Plan de energía equilibrado o alto rendimiento durante compilaciones.
- Driver NVIDIA Studio estable y actualizado.
- Al menos 250 GB libres antes de instalar el proyecto y sus cachés.

## 3. Unreal Engine

Instalar Epic Games Launcher y **Unreal Engine 5.7**.

Componentes:

- Core Components.
- Starter Content.
- Templates and Feature Packs.
- Soporte de plataforma Windows.

No instalar inicialmente:

- Símbolos completos de depuración del editor.
- Soporte Android, iOS, Linux o consolas.
- Engine source.
- Plugins de terceros.

No actualizar la versión del motor durante el MVP.

## 4. Visual Studio

Instalar **Visual Studio 2022 Community 17.14**.

Workloads:

- Desktop development with C++.
- Game development with C++.

Componentes:

- MSVC v143.
- Windows 11 SDK.
- Visual Studio Tools for Unreal Engine.
- C++ profiling tools.
- C++ AddressSanitizer.
- CMake tools, si son incluidos por el workload.

Después de instalar, reiniciar Windows.

## 5. Control de versiones

Instalar:

- Git for Windows.
- Git LFS.

Verificación esperada:

```powershell
git --version
git lfs version
```

El agente configurará `.gitignore` y `.gitattributes` dentro del repositorio.

## 6. Agente

Instalar Codex para Windows o Codex CLI y autenticarlo. Abrir el agente desde la raíz del repositorio, por ejemplo:

```powershell
cd C:\GameDev\Astraeon
```

El agente necesita permiso para:

- Leer y escribir dentro del repositorio.
- Ejecutar PowerShell y procesos de compilación.
- Invocar UnrealBuildTool, AutomationTool y UnrealEditor.
- Crear procesos de prueba y empaquetado.

No necesita acceso irrestricto a otras carpetas personales.

## 7. Estructura inicial

Copiar este paquete en:

```text
C:\GameDev\Astraeon\
├── AGENTS.md
├── PROMPT_INICIAL_DESARROLLO.md
├── README.md
└── Docs\
    ├── GAME_DESIGN_MASTER.md
    ├── MVP_LA_PRIMERA_SENAL.md
    └── WINDOWS_11_SETUP.md
```

No es necesario crear manualmente el proyecto Unreal. El prompt inicial ordena al agente verificar el entorno y realizar el bootstrap.

## 8. Verificaciones previas

Ejecutar o permitir que el agente ejecute:

```powershell
Get-ComputerInfo | Select-Object WindowsProductName, WindowsVersion, OsArchitecture
git --version
git lfs version
```

Verificar también:

- Unreal Engine 5.7 aparece instalado en Epic Games Launcher.
- Visual Studio Installer muestra los workloads requeridos.
- El SSD conserva al menos 250 GB libres.
- Windows Defender permite compilar dentro de `C:\GameDev\Astraeon`.

No desactivar el antivirus globalmente. Si existe una penalización comprobada, evaluar una exclusión limitada a cachés de compilación, nunca a carpetas amplias sin medición previa.

## 9. Primera ejecución

1. Reiniciar Windows tras completar instalaciones.
2. Abrir Epic Games Launcher una vez y verificar Unreal Engine 5.7.
3. Abrir Visual Studio Installer y confirmar componentes.
4. Copiar el paquete documental a `C:\GameDev\Astraeon`.
5. Abrir Codex en esa carpeta.
6. Entregarle el contenido de `PROMPT_INICIAL_DESARROLLO.md`.
7. Permitir que continúe hasta terminar el MVP o encontrar un bloqueo real.

## 10. Comportamiento esperado del agente

El agente deberá:

- Diagnosticar rutas y versiones.
- Crear el proyecto C++.
- Compilar una base vacía.
- Configurar Git y Git LFS.
- Crear backlog y estado.
- Implementar por hitos.
- Ejecutar pruebas y builds.
- Corregir automáticamente errores reproducibles.
- Preguntar sólo ante bloqueos definidos en `AGENTS.md`.

## 11. Intervenciones que todavía pueden ser necesarias

Aunque el desarrollo sea autónomo, Windows o Epic pueden requerir intervención para:

- Iniciar sesión.
- Aceptar una licencia.
- Confirmar instalación del motor.
- Autorizar un componente de Visual Studio.
- Reiniciar el equipo.
- Resolver un diálogo del sistema que bloquee la automatización.

Estas acciones son de entorno, no decisiones de diseño.

## 12. Criterio de entorno listo

El entorno está preparado cuando:

- Unreal Engine 5.7 inicia.
- Visual Studio puede compilar un proyecto C++ de Unreal.
- Git y Git LFS responden.
- El agente puede leer y escribir el repositorio.
- UnrealBuildTool y AutomationTool pueden ejecutarse.
- Existe espacio suficiente.
- El prompt inicial está disponible en la raíz.

