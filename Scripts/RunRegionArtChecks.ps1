param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('Automation','Visual','Critical','Package','PackagedVisual','PackagedCritical')]
    [string]$Check
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$engine = 'C:\Program Files\Epic Games\UE_5.7'
$project = Join-Path $repo 'Astraeon.uproject'
$log = Join-Path $repo "Saved\Logs\RegionArt_$Check.log"
$stdout = Join-Path $repo "Saved\Logs\RegionArt_$Check.stdout.log"
$stderr = Join-Path $repo "Saved\Logs\RegionArt_$Check.stderr.log"
$exe = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$arguments = @($project, '/Game/Maps/L_AstraeonBootstrap', '-unattended', '-nosplash', '-nop4', "-abslog=$log")
$success = 'AstraeonRegionArtSmoke: Passed=true'
switch ($Check) {
    'Automation' {
        $arguments += @('-nullrhi', '-ExecCmds="Automation RunTests Astraeon."', '-TestExit="Automation Test Queue Empty"', "-ReportExportPath=$repo\Saved\Automation\RegionArt")
        $success = 'Test Completed. Result=\{Success\} Name=\{PresentationPreservesGameplay\}'
    }
    'Visual' { $arguments += @('-game','-RenderOffscreen','-windowed','-ResX=1920','-ResY=1080','-AstraeonSmokeRegionArt','-AstraeonRegionArtScreenshots','-AstraeonSeed=13579') }
    'Critical' {
        $arguments += @('-game','-nullrhi','-AstraeonAutoSmokeCriticalPath','-AstraeonSeed=13579')
        $success = 'AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true'
    }
    'Package' {
        $exe = Join-Path $engine 'Engine\Build\BatchFiles\RunUAT.bat'
        $arguments = @('BuildCookRun',"-project=$project",'-noP4','-platform=Win64','-clientconfig=Development','-build','-cook','-stage','-pak','-archive',"-archivedirectory=$repo\Builds\WindowsRegionArt",'-utf8output')
        $success = 'BUILD SUCCESSFUL'
    }
    'PackagedVisual' {
        $exe = Join-Path $repo 'Builds\WindowsRegionArt\Astraeon\Binaries\Win64\Astraeon.exe'
        $arguments = @('-unattended','-nosplash','-RenderOffscreen','-windowed','-ResX=1920','-ResY=1080','-AstraeonSmokeRegionArt','-AstraeonRegionArtScreenshots','-AstraeonSeed=13579',"-abslog=$log")
    }
    'PackagedCritical' {
        $exe = Join-Path $repo 'Builds\WindowsRegionArt\Astraeon\Binaries\Win64\Astraeon.exe'
        $arguments = @('-nullrhi','-unattended','-nosplash','-AstraeonAutoSmokeCriticalPath','-AstraeonSeed=13579',"-abslog=$log")
        $success = 'AstraeonCriticalPathSmoke: Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true'
    }
}
$process = Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $repo -PassThru -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr
$processHandle = $process.Handle
$deadline = (Get-Date).AddMinutes(20)
while (-not $process.WaitForExit(10000)) {
    if ((Get-Date) -gt $deadline) {
        Stop-Process -Id $process.Id
        throw "Timed out: $Check; stopped only the process launched by this check"
    }
}
$exitCode = $process.ExitCode
$evidenceLog = if ($Check -eq 'Package') { $stdout } else { $log }
$content = Get-Content -Raw -LiteralPath $evidenceLog
$passed = ($exitCode -eq 0) -and ($content -match $success) -and ($content -notmatch 'Result=\{Fail\}|Fatal error:')
$result = [ordered]@{ check=$Check; passed=$passed; exit_code=$exitCode; log=$evidenceLog; date=(Get-Date -Format o) }
if ($Check -eq 'Automation') { $result.tests_passed = [regex]::Matches($content,'Test Completed. Result=\{Success\}').Count }
$result | ConvertTo-Json | Set-Content -Encoding UTF8 -LiteralPath (Join-Path $repo "ContentPipeline\reports\region_$Check.json")
Write-Output "REGION_ART_CHECK $Check Passed=$passed Exit=$exitCode"
if (-not $passed) { throw "Check failed: $evidenceLog" }
