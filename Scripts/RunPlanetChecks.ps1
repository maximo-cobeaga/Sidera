param(
    [ValidateSet('Map','Walk','Cardinals','Automation','Critical')][string]$Check='Automation',
    [double]$RadiusCm=20000,
    [int]$Seconds=250,
    [switch]$Profile
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$editor='C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$label="Planet_${Check}_${RadiusCm}"
$log=Join-Path $repo "Saved\Logs\$label.log"
$arguments=@("$repo\Astraeon.uproject",'-unattended','-nosplash','-nop4',"-abslog=$log")
switch ($Check) {
    'Map' { $arguments+=@('-RenderOffscreen',"-ExecutePythonScript=$repo\Scripts\Editor\CreateCubeSphereTestMap.py") }
    'Automation' { $arguments+=@('-nullrhi','-ExecCmds="Automation RunTests Astraeon"','-TestExit="Automation Test Queue Empty"') }
    'Critical' { $arguments+=@('/Game/Maps/L_AstraeonBootstrap','-game','-nullrhi','-AstraeonAutoSmokeCriticalPath','-AstraeonSeed=13579') }
    default {
        $arguments+=@('/Game/Maps/TL_11_CubeSphereClosed','-game','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080',"-AstraeonPlanetRadiusCm=$RadiusCm")
        if ($Check -eq 'Walk') { $arguments+=@('-AstraeonSmokePlanetWalk',"-AstraeonWalkSeconds=$Seconds") }
        else { $arguments+='-AstraeonSmokePlanetCardinals' }
        if ($Profile) { $arguments+=@('-AstraeonPerfBaseline','-AstraeonPerfWarmup=10','-AstraeonPerfSeconds=30','-AstraeonPerfKeepRunning') }
    }
}
$p=Start-Process -FilePath $editor -ArgumentList $arguments -WorkingDirectory $repo -PassThru -WindowStyle Hidden -RedirectStandardOutput "$log.stdout" -RedirectStandardError "$log.stderr"
$handle=$p.Handle
if (-not $p.WaitForExit(600000)) { Stop-Process -Id $p.Id; throw "Timeout: $label (only owned process stopped)" }
$content=Get-Content -Raw -LiteralPath $log
if ($p.ExitCode -ne 0 -or $content -match 'Result=\{Fail|Fatal error:|LogPython: Error:') { throw "Failed: $label exit=$($p.ExitCode); $log" }
$expected=switch($Check) {
    'Map' {'ASTRAEON_CUBE_SPHERE_MAP: PASS'}
    'Walk' {'AstraeonPlanetWalk: RESULTADO=OK'}
    'Cardinals' {'PlanetCardinals: PASS'}
    'Critical' {'Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true'}
    'Automation' {'Astraeon.Planet.Surface.ContinuityAndInvalidInput'}
}
if ($content -notmatch [regex]::Escape($expected)) { throw "Missing completion marker: $label" }
$successes=[regex]::Matches($content,'Test Completed. Result=\{Success\}').Count
if ($Check -eq 'Automation' -and $successes -lt 75) { throw "Incomplete queue: $successes tests" }
Write-Output "PLANET_CHECK $Check PASS tests=$successes log=$log"
