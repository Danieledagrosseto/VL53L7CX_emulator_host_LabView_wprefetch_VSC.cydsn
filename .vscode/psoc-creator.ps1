param(
    [ValidateSet('Build', 'Rebuild', 'Clean', 'Open')]
    [string]$Action = 'Build',
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$workspace = Join-Path $projectRoot 'VL53L7CX_emulator_host_LabView_wprefetch_VSC.cywrk'

if (-not (Test-Path -LiteralPath $workspace -PathType Leaf)) {
    throw "PSoC Creator workspace not found: $workspace"
}

$creatorRoot = $env:PSOC_CREATOR_HOME
if (-not $creatorRoot) {
    $creatorRoot = Join-Path ${env:ProgramFiles(x86)} 'Cypress\PSoC Creator\4.4\PSoC Creator'
}

if ($Action -eq 'Open') {
    $creator = Join-Path $creatorRoot 'bin\psoc_creator.exe'
    if (-not (Test-Path -LiteralPath $creator -PathType Leaf)) {
        throw "PSoC Creator not found: $creator. Set PSOC_CREATOR_HOME to the PSoC Creator installation directory."
    }
    Start-Process -FilePath $creator -ArgumentList "`"$workspace`""
    exit 0
}

$manager = Join-Path $creatorRoot 'bin\cyprjmgr.exe'
if (-not (Test-Path -LiteralPath $manager -PathType Leaf)) {
    throw "PSoC Creator build manager not found: $manager. Set PSOC_CREATOR_HOME to the PSoC Creator installation directory."
}

Push-Location $projectRoot
try {
    & $manager -wrk $workspace "-$($Action.ToLowerInvariant())" -c $Configuration -t 'ARM GCC 5.4-2016-q2-update'
    $buildExitCode = $LASTEXITCODE
}
finally {
    Pop-Location
}
exit $buildExitCode
