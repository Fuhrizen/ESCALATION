param(
    [ValidateSet('linux', 'windows', 'all')]
    [string]$Platform = 'linux'
)

$ErrorActionPreference = 'Stop'
$targets = if ($Platform -eq 'all') { @('linux', 'windows') } else { @($Platform) }
Push-Location $PSScriptRoot
try {
    foreach ($target in $targets) {
        & docker compose up --build --exit-code-from $target $target
        if ($LASTEXITCODE -ne 0) { throw "Build failed for $target (exit $LASTEXITCODE). Artifacts were not extracted." }

        $destination = Join-Path $PSScriptRoot "dist-$target"
        New-Item -ItemType Directory -Force -Path $destination | Out-Null
        & docker compose cp "${target}:/out/${target}/." $destination
        if ($LASTEXITCODE -ne 0) { throw "Artifact extraction failed for $target (exit $LASTEXITCODE)." }
        Write-Host "Build and extraction complete: $destination"
    }
}
finally {
    Pop-Location
}
