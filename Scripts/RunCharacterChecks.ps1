param(
    [ValidateSet('Audit','Repair','Automation','Visual','VisualFP','Package','PackagedVisual','Critical','PackagedCritical')]
    [string]$Check = 'Automation'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$engine = 'C:\Program Files\Epic Games\UE_5.7\Engine'
$exe = Join-Path $engine 'Binaries\Win64\UnrealEditor-Cmd.exe'
$log = Join-Path $repo "Saved\Logs\Character_$Check.log"
$arguments = @((Join-Path $repo 'Astraeon.uproject'), '-unattended', '-nop4', '-nosplash', "-abslog=$log")
switch ($Check) {
    'Audit' { $arguments += @('-nullrhi', "-ExecutePythonScript=$repo\Scripts\Editor\AuditCharacterPose.py") }
    'Repair' { $arguments += @('-nullrhi', "-ExecutePythonScript=$repo\Scripts\Editor\RepairCharacterPresentation.py") }
    'Automation' { $arguments += @('-nullrhi', '-ExecCmds="Automation RunTests Astraeon"', '-TestExit="Automation Test Queue Empty"') }
    'Package' {
        $exe = Join-Path $engine 'Build\BatchFiles\RunUAT.bat'
        $arguments = @('BuildCookRun',"-project=$repo\Astraeon.uproject",'-noP4','-platform=Win64','-clientconfig=Development','-build','-cook','-stage','-pak','-archive',"-archivedirectory=$repo\Builds\WindowsProtagonista",'-utf8output')
    }
    { $_ -in 'Critical','PackagedCritical' } { $arguments += @('/Game/Maps/L_AstraeonBootstrap','-game','-nullrhi','-AstraeonAutoSmokeCriticalPath','-AstraeonSeed=13579') }
    default {
        $arguments += @('/Game/Maps/L_AstraeonBootstrap','-game','-RenderOffscreen','-windowed','-ResX=1280','-ResY=720','-AstraeonCameraShot')
        if ($Check -ne 'VisualFP') { $arguments += '-AstraeonStartThirdPerson' }
    }
}
if ($Check.StartsWith('Packaged')) {
    $exe = Join-Path $repo 'Builds\WindowsProtagonista\Astraeon\Binaries\Win64\Astraeon.exe'
    $arguments = $arguments | Select-Object -Skip 1
}
$stdout = Join-Path $repo "Saved\Logs\Character_$Check.stdout.log"
$stderr = Join-Path $repo "Saved\Logs\Character_$Check.stderr.log"
$process = Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $repo -PassThru -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr
$handle = $process.Handle
if (-not $process.WaitForExit(1200000)) {
    Stop-Process -Id $process.Id
    throw "Character check timed out: $Check (stopped only the process launched here)"
}
Write-Output "CHARACTER_CHECK $Check Exit=$($process.ExitCode) Log=$log"
if ($process.ExitCode -ne 0) { throw "Character check failed: $Check" }
if ($Check -ne 'Package') {
    $content = Get-Content -Raw -LiteralPath $log
    if ($content -match 'LogPython: Error:|Result=\{Fail|Fatal error:') { throw "Failure in log: $log" }
    if ($Check -like '*Visual*' -and ([regex]::Matches($content, 'AstraeonCharacterViewSmoke: Passed=true').Count -ne 2)) { throw "Both camera input transitions must pass: $log" }
    if ($Check -like '*Critical' -and $content -notmatch 'Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true') { throw "Critical path did not pass: $log" }
    if ($Check -eq 'Automation' -and ([regex]::Matches($content, 'Test Completed. Result=\{Success\}').Count -lt 55)) { throw "Automation queue incomplete: $log" }
} else {
    if ((Get-Content -Raw -LiteralPath $stdout) -notmatch 'BUILD SUCCESSFUL') { throw "Package did not succeed: $stdout" }
}
