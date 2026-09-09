param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('Automation','InputBootstrap','InputArt','Visual','Package','PackagedInput','PackagedVisual','CriticalPath','PackagedCritical')]
    [string]$Check
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$engine = 'C:\Program Files\Epic Games\UE_5.7'
$project = Join-Path $repo 'Astraeon.uproject'
$log = Join-Path $repo "Saved\Logs\Itaca_$Check.log"
$exe = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$arguments = @($project, '-unattended', '-nosplash', '-nop4', "-abslog=$log")
$success = 'AstraeonItacaInputSmoke: Passed=true'
switch ($Check) {
    'Automation' {
        $arguments += @('-nullrhi', '-ExecCmds="Automation RunTests Astraeon."', '-TestExit="Automation Test Queue Empty"', "-ReportExportPath=$repo\Saved\Automation\Itaca")
        $success = 'Test Completed. Result=\{Success\}'
    }
    'InputBootstrap' { $arguments += @('/Game/Maps/L_AstraeonBootstrap','-game','-nullrhi','-AstraeonSmokeItacaInput','-AstraeonSeed=13579') }
    'InputArt' { $arguments += @('/Game/Maps/TL_Art_MVP','-game','-nullrhi','-AstraeonSmokeItacaInput','-AstraeonSeed=13579') }
    'Visual' { $arguments += @('/Game/Maps/L_AstraeonBootstrap','-game','-RenderOffscreen','-windowed','-ResX=1280','-ResY=720','-AstraeonSmokeItacaInput','-AstraeonArtScreenshot','-AstraeonSeed=13579') }
    'CriticalPath' {
        $arguments += @('/Game/Maps/L_AstraeonBootstrap','-game','-nullrhi','-AstraeonAutoSmokeCriticalPath','-AstraeonSeed=13579')
        $success = 'AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true'
    }
    'Package' {
        $exe = Join-Path $engine 'Engine\Build\BatchFiles\RunUAT.bat'
        $arguments = @('BuildCookRun',"-project=$project",'-noP4','-platform=Win64','-clientconfig=Development','-build','-cook','-stage','-pak','-archive',"-archivedirectory=$repo\Builds\WindowsItaca",'-utf8output')
        $success = 'BUILD SUCCESSFUL'
    }
    'PackagedInput' {
        $exe = Join-Path $repo 'Builds\WindowsItaca\Astraeon\Binaries\Win64\Astraeon.exe'
        $arguments = @('-nullrhi','-unattended','-nosplash','-AstraeonSmokeItacaInput','-AstraeonSeed=13579',"-abslog=$log")
    }
    'PackagedVisual' {
        $exe = Join-Path $repo 'Builds\WindowsItaca\Astraeon\Binaries\Win64\Astraeon.exe'
        $arguments = @('-unattended','-nosplash','-RenderOffscreen','-windowed','-ResX=1280','-ResY=720','-AstraeonSmokeItacaInput','-AstraeonArtScreenshot','-AstraeonSeed=13579',"-abslog=$log")
    }
    'PackagedCritical' {
        $exe = Join-Path $repo 'Builds\WindowsItaca\Astraeon\Binaries\Win64\Astraeon.exe'
        $arguments = @('-nullrhi','-unattended','-nosplash','-AstraeonAutoSmokeCriticalPath','-AstraeonSeed=13579',"-abslog=$log")
        $success = 'AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true'
    }
}
$stdout = Join-Path $repo "Saved\Logs\Itaca_$Check.stdout.log"
$stderr = Join-Path $repo "Saved\Logs\Itaca_$Check.stderr.log"
$process = Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $repo -PassThru -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr
$processHandle = $process.Handle # Retain the handle so Windows PowerShell can read ExitCode after exit.
if (-not $process.WaitForExit(1200000)) {
    Stop-Process -Id $process.Id
    throw "Timed out in $Check; only the process created by this script was stopped"
}
$exitCode = $process.ExitCode
$content = if ($Check -eq 'Package') { Get-Content -Raw -LiteralPath $stdout } else { Get-Content -Raw -LiteralPath $log }
$passed = ($exitCode -eq 0) -and ($content -match $success)
if ($Check -eq 'Automation') { $passed = $passed -and ($content -notmatch 'Result=\{Fail') -and ([regex]::Matches($content,$success).Count -ge 35) }
$result = [ordered]@{ check=$Check; passed=$passed; exit_code=$exitCode; log="Saved/Logs/Itaca_$Check.log"; date=(Get-Date -Format o) }
if ($Check -eq 'Package') { $result.log = "Saved/Logs/Itaca_$Check.stdout.log" }
$result | ConvertTo-Json | Set-Content -Encoding UTF8 -LiteralPath (Join-Path $repo "ContentPipeline\reports\itaca_$Check.json")
Write-Output "ITACA_CHECK $Check Passed=$passed Exit=$exitCode"
if (-not $passed) { throw "Check failed; inspect $log and $stdout" }
