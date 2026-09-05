@echo off
setlocal
set "PROJECT_DIR=C:\Users\MAXIMO\Desktop\Astraeon"
set "UE_DIR=C:\Program Files\Epic Games\UE_5.7"
set "RESULT=%PROJECT_DIR%\Saved\Logs\AstraeonRebuildResult3.txt"

if not exist "%PROJECT_DIR%\Saved\Logs" mkdir "%PROJECT_DIR%\Saved\Logs"

echo ASTRAEON REBUILD3 START %DATE% %TIME% > "%RESULT%"
echo (Sin automation tests para evitar cuelgue en exit de UnrealEditor-Cmd) >> "%RESULT%"

echo [1/3] UBT AstraeonEditor >> "%RESULT%"
"%UE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" AstraeonEditor Win64 Development -Project="%PROJECT_DIR%\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE >> "%RESULT%" 2>&1
echo EXITCODE_EDITOR_BUILD=%ERRORLEVEL% >> "%RESULT%"

echo [2/3] UBT Astraeon game >> "%RESULT%"
"%UE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" Astraeon Win64 Development -Project="%PROJECT_DIR%\Astraeon.uproject" -WaitMutex -NoHotReloadFromIDE >> "%RESULT%" 2>&1
echo EXITCODE_GAME_BUILD=%ERRORLEVEL% >> "%RESULT%"

echo [3/3] BuildCookRun package >> "%RESULT%"
call "%UE_DIR%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%PROJECT_DIR%\Astraeon.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="%PROJECT_DIR%\Builds\WindowsDevelopment" -utf8output >> "%RESULT%" 2>&1
echo EXITCODE_PACKAGE=%ERRORLEVEL% >> "%RESULT%"

echo ASTRAEON REBUILD3 DONE %DATE% %TIME% >> "%RESULT%"
echo DONE_MARKER_rebuild3 >> "%RESULT%"
