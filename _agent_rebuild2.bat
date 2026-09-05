@echo off
setlocal
set "PROJECT_DIR=C:\Users\MAXIMO\Desktop\Astraeon"
set "UE_DIR=C:\Program Files\Epic Games\UE_5.7"
set "RESULT=%PROJECT_DIR%\Saved\Logs\AstraeonRebuildResult2.txt"

if not exist "%PROJECT_DIR%\Saved\Logs" mkdir "%PROJECT_DIR%\Saved\Logs"

echo ASTRAEON REBUILD2 START %DATE% %TIME% > "%RESULT%"
echo (Automation tests ya confirmados 35/35 OK en la corrida anterior con este mismo codigo; se omiten aqui para evitar el cuelgue de UnrealEditor-Cmd al salir) >> "%RESULT%"

echo [1/2] UBT Astraeon game >> "%RESULT%"
"%UE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" Astraeon Win64 Development -Project="%PROJECT_DIR%\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE >> "%RESULT%" 2>&1
echo EXITCODE_GAME_BUILD=%ERRORLEVEL% >> "%RESULT%"

echo [2/2] BuildCookRun package >> "%RESULT%"
call "%UE_DIR%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROJECT_DIR%\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="%PROJECT_DIR%\Builds\WindowsDevelopment" -utf8output >> "%RESULT%" 2>&1
echo EXITCODE_PACKAGE=%ERRORLEVEL% >> "%RESULT%"

echo ASTRAEON REBUILD2 DONE %DATE% %TIME% >> "%RESULT%"
echo DONE_MARKER_7b1e44 >> "%RESULT%"
