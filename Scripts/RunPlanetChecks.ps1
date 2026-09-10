param(
    [ValidateSet('Map','Walk','Cardinals','Automation','Critical','PatchLODMap','PatchLOD','Observer','LabMap','State')][string]$Check='Automation',
    # Walk, Cardinals and LabMap: which planet lab. Each map carries its radius as data.
    [ValidateSet('TL_11_CubeSphereClosed','TL_13_CollisionRing','TL_14_FrameTransition')][string]$Map='TL_11_CubeSphereClosed',
    [double]$RadiusCm=20000,
    [int]$Seconds=250,
    [switch]$Profile,
    [switch]$Trace,
    # Extra engine arguments for one run, e.g. '-AstraeonNoFrameShift' for an A/B comparison.
    [string[]]$Extra=@()
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$editor='C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
# Maps carry their radius as data; only an explicit -RadiusCm overrides it.
$radiusArg=if ($PSBoundParameters.ContainsKey('RadiusCm')) { "-AstraeonPlanetRadiusCm=$RadiusCm" } else { $null }
$suffix=($Extra | ForEach-Object { $_.TrimStart('-') }) -join '_'
$label=switch -Wildcard ($Check) {
    'PatchLOD*' { "Planet_$Check" }
    'Observer' { "Planet_$Check" }
    'State' { "Planet_$Check" }
    default { "Planet_${Check}_${Map}$(if ($radiusArg) {"_$RadiusCm"})$(if ($suffix) {"_$suffix"})" }
}
$log=Join-Path $repo "Saved\Logs\$label.log"
$arguments=@("$repo\Astraeon.uproject",'-unattended','-nosplash','-nop4',"-abslog=$log")
$render=@('-game','-RenderOffscreen','-windowed','-ForceRes','-ResX=1920','-ResY=1080')
switch ($Check) {
    'Map' { $arguments+=@('-RenderOffscreen',"-ExecutePythonScript=$repo\Scripts\Editor\CreateCubeSphereTestMap.py") }
    'PatchLODMap' { $arguments+=@('-RenderOffscreen',"-ExecutePythonScript=$repo\Scripts\Editor\CreatePatchLODTestMap.py") }
    'LabMap' { $arguments+=@('-RenderOffscreen',"-ExecutePythonScript=`"$repo\Scripts\Editor\CreatePlanetLabMap.py $Map`"") }
    'Automation' { $arguments+=@('-nullrhi','-ExecCmds="Automation RunTests Astraeon"','-TestExit="Automation Test Queue Empty"') }
    'Critical' { $arguments+=@('/Game/Maps/L_AstraeonBootstrap','-game','-nullrhi','-AstraeonAutoSmokeCriticalPath','-AstraeonSeed=13579') }
    'PatchLOD' {
        $arguments+=@('/Game/Maps/TL_12_PatchLOD')+$render+'-AstraeonSmokePatchLOD'
        if ($PSBoundParameters.ContainsKey('RadiusCm')) { $arguments+="-AstraeonPlanetRadiusCm=$RadiusCm" }
        if ($Profile) { $arguments+=@('-AstraeonPerfBaseline','-AstraeonPerfWarmup=5','-AstraeonPerfSeconds=170','-AstraeonPerfKeepRunning') }
    }
    'Observer' { $arguments+=@('/Game/Maps/TL_12_PatchLOD')+$render+'-AstraeonSmokePlanetObserver' }
    'State' { $arguments+=@('/Game/Maps/TL_13_CollisionRing')+$render+@('-AstraeonSmokePlanetState','-AstraeonPlanetFauna') }
    default {
        $arguments+=@("/Game/Maps/$Map")+$render
        if ($radiusArg) { $arguments+=$radiusArg }
        if ($Check -eq 'Walk') { $arguments+=@('-AstraeonSmokePlanetWalk',"-AstraeonWalkSeconds=$Seconds") }
        else { $arguments+='-AstraeonSmokePlanetCardinals' }
        if ($Profile) { $arguments+=@('-AstraeonPerfBaseline','-AstraeonPerfWarmup=10','-AstraeonPerfSeconds=30','-AstraeonPerfKeepRunning') }
    }
}
$arguments+=$Extra
if ($Trace) {
    # Unreal Insights capture; the file stays in Saved/Profiling, never in the repository.
    $traceFile=Join-Path $repo "Saved\Profiling\$label.utrace"
    New-Item -ItemType Directory -Force (Split-Path $traceFile) | Out-Null
    # The engine will not overwrite an existing trace: a stale file would be silently re-read.
    if (Test-Path $traceFile) { Remove-Item -LiteralPath $traceFile }
    $arguments+=@('-trace=cpu,frame,bookmark,log',"-tracefile=$traceFile")
}
$p=Start-Process -FilePath $editor -ArgumentList $arguments -WorkingDirectory $repo -PassThru -WindowStyle Hidden -RedirectStandardOutput "$log.stdout" -RedirectStandardError "$log.stderr"
$handle=$p.Handle
if (-not $p.WaitForExit(600000)) { Stop-Process -Id $p.Id; throw "Timeout: $label (only owned process stopped)" }
$content=Get-Content -Raw -LiteralPath $log
# An engine ensure fails the run: a handled ensure is still broken engine state (P2.6 found one
# per session with every local frame shift, invisible to smokes that only check behaviour).
if ($p.ExitCode -ne 0 -or $content -match 'Result=\{Fail|Fatal error:|LogPython: Error:|Ensure condition failed') { throw "Failed: $label exit=$($p.ExitCode); $log" }
$expected=switch($Check) {
    'Map' {'ASTRAEON_CUBE_SPHERE_MAP: PASS'}
    'PatchLODMap' {'ASTRAEON_PATCH_LOD_MAP: PASS'}
    'LabMap' {"ASTRAEON_PLANET_LAB_MAP: PASS /Game/Maps/$Map"}
    'Walk' {'AstraeonPlanetWalk: RESULTADO=OK'}
    'Cardinals' {'PlanetCardinals: PASS'}
    'PatchLOD' {'PlanetPatchLOD: RESULTADO=OK'}
    'Observer' {'PlanetObserver: RESULTADO=OK'}
    'State' {'PlanetState: RESULTADO=OK'}
    'Critical' {'Deployed=true Scanned=true Crafted=true Resolved=true Saved=true Loaded=true LoadedResolved=true'}
    'Automation' {'Astraeon.Planet.Surface.ContinuityAndInvalidInput'}
}
if ($content -notmatch [regex]::Escape($expected)) { throw "Missing completion marker: $label" }
$successes=[regex]::Matches($content,'Test Completed. Result=\{Success\}').Count
if ($Check -eq 'Automation') {
    $discovered=[regex]::Match($content,"Found (\d+) automation tests based on 'Astraeon'")
    if (-not $discovered.Success -or $successes -lt 96 -or $successes -ne [int]$discovered.Groups[1].Value) {
        throw "Incomplete queue: $successes successful tests; expected all discovered tests and at least 96"
    }
    foreach ($required in @(
        'Astraeon.Planet.Patches.AddressHierarchy',
        'Astraeon.Planet.Patches.StableHashGoldenVectors',
        'Astraeon.Planet.LOD.NeighborsAcrossAllFaces',
        'Astraeon.Planet.LOD.BalancedDeterministicEngineeringTiers',
        'Astraeon.Planet.LOD.RejectsHolesOverlapsAndUnbalancedTrees',
        'Astraeon.Planet.LOD.SkirtsCoverCoarseNeighbourSeams',
        'Astraeon.Planet.LOD.FastSelectionMatchesReference',
        'Astraeon.Planet.Patches.MeshBudgetAndPrecision',
        'Astraeon.Planet.Patches.SharedEdgesAndLodSamples',
        'Astraeon.Planet.Patches.InvalidAndCancelledBuilds',
        'Astraeon.Planet.Streaming.RevisionsCancellationAndBackpressure',
        'Astraeon.Planet.Streaming.SubmissionOrderDeterminism',
        'Astraeon.Planet.PatchManager.HoleFreeSplitAndMerge',
        'Astraeon.Planet.PatchManager.RejectsStaleFailedAndInvalid',
        'Astraeon.Planet.PatchManager.DeterministicRouteWithWorkers',
        'Astraeon.Planet.Collision.MatchesRenderedSurface',
        'Astraeon.Planet.Collision.RingCoversCap',
        'Astraeon.Planet.PatchManager.RoundTripRegeneratesSamePatch',
        'Astraeon.Planet.Entities.DeterministicPlacement',
        'Astraeon.Planet.State.DefeatSurvivesUnloadReloadAndSave',
        'Astraeon.Persistence.SaveGame.V3PlanetaryLocation'
    )) {
        if ($content -notmatch ('Test Completed\. Result=\{Success\}[^\r\n]*Path=\{' + [regex]::Escape($required) + '\}')) {
            throw "Missing required Phase 2 test: $required"
        }
    }
}
Write-Output "PLANET_CHECK $Check PASS tests=$successes log=$log"
