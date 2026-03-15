#Requires -Version 5.1
# Setup.ps1 - Main setup logic. Run from repo root (invoked by Setup.bat with admin).

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

# Install Graphic Tools for debugging only if not already installed
$graphicCap = 'Tools.Graphics.DirectX~~~~0.0.1.0'
$installed = $false
try {
    $cap = Get-WindowsCapability -Online -Name $graphicCap -ErrorAction SilentlyContinue
    if ($cap -and $cap.State -eq 'Installed') { $installed = $true }
} catch { }
if (-not $installed) {
    Write-Host '[Install Graphic Tools for debugging]'
    dism /online /add-capability /capabilityname:$graphicCap 2>$null
} else {
    Write-Host '[Graphic Tools for debugging already installed]'
}

# Find Visual Studio
$VSPath = $null
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $VSPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
}
$fallbacks = @(
    "$env:ProgramFiles\Microsoft Visual Studio\2026\BuildTools",
    "$env:ProgramFiles\Microsoft Visual Studio\2026\Community",
    "$env:ProgramFiles\Microsoft Visual Studio\2026\Professional",
    "$env:ProgramFiles\Microsoft Visual Studio\2026\Enterprise",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools"
)
foreach ($p in $fallbacks) {
    if (-not $VSPath -and (Test-Path $p)) { $VSPath = $p; break }
}

if (-not $VSPath) {
    Write-Host '[No Visual Studio detected - Installing Visual Studio 2026 Build Tools]'
    $vsExe = Join-Path $env:TEMP 'vsbuildtool.exe'
    Invoke-WebRequest -Uri 'https://aka.ms/vs/stable/vs_buildtools.exe' -UseBasicParsing -OutFile $vsExe
    $args = @(
        '--passive', '--wait',
        '--add', 'Microsoft.VisualStudio.Workload.VCTools;includeRecommended',
        '--add', 'Microsoft.VisualStudio.Component.Windows11SDK.26100',
        '--add', 'Microsoft.VisualStudio.Component.VC.CMake.Project',
        '--add', 'Microsoft.VisualStudio.Workload.MSBuildTools;includeRecommended',
        '--add', 'Microsoft.VisualStudio.Workload.ManagedDesktopBuildTools;includeRecommended',
        '--add', 'Microsoft.NetCore.Component.Runtime.9.0',
        '--add', 'Microsoft.NetCore.Component.SDK',
        '--add', 'Microsoft.VisualStudio.Component.Vcpkg',
        '--add', 'Microsoft.VisualStudio.Component.VC.CLI.Support',
        '--add', 'Microsoft.VisualStudio.Component.VC.ATLMFC',
        '--addProductLang', 'en-us'
    )
    $p = Start-Process -FilePath $vsExe -ArgumentList $args -Wait -PassThru -NoNewWindow
    if ($p.ExitCode -ne 0) { Write-Warning "VS installer exited with $($p.ExitCode)" }
    if (Test-Path "$env:ProgramFiles\Microsoft Visual Studio\2026\BuildTools") {
        $VSPath = "$env:ProgramFiles\Microsoft Visual Studio\2026\BuildTools"
    }
}

if (-not $VSPath) {
    Write-Error 'No Visual Studio or Build Tools installation has been found.'
    exit 1
}

# Find dotnet (same order as original batch: LocalAppData, ProgramW6432, ProgramFiles)
$DOTNET_EXE = $null
$dotnetPaths = @(
    "$env:LocalAppData\Microsoft\dotnet\dotnet.exe",
    "$env:ProgramFiles\dotnet\dotnet.exe"
)
if ($env:ProgramW6432) {
    $dotnetPaths = @(
        "$env:LocalAppData\Microsoft\dotnet\dotnet.exe",
        "$env:ProgramW6432\dotnet\dotnet.exe",
        "$env:ProgramFiles\dotnet\dotnet.exe"
    )
}
foreach ($d in $dotnetPaths) {
    if (Test-Path $d) { $DOTNET_EXE = $d; break }
}
if (-not $DOTNET_EXE) {
    $cmd = Get-Command dotnet -ErrorAction SilentlyContinue; if ($cmd) { $DOTNET_EXE = $cmd.Source }
}

# Install .NET SDK if no SDK found
$haveSdk = $false
if ($DOTNET_EXE) {
    try { $null = & $DOTNET_EXE --list-sdks 2>$null; $haveSdk = $? } catch { }
}
if (-not $haveSdk) {
    Write-Host '[No .NET SDK found - Installing .NET 9.0 SDK]'
    $installPs1 = Join-Path $env:TEMP 'dotnet-install.ps1'
    Invoke-WebRequest -Uri 'https://dot.net/v1/dotnet-install.ps1' -UseBasicParsing -OutFile $installPs1
    & $installPs1 -Channel 9.0 -NoPath
}

# Re-find dotnet
$DOTNET_EXE = $null
foreach ($d in $dotnetPaths) {
    if (Test-Path $d) { $DOTNET_EXE = $d; break }
}
if (-not $DOTNET_EXE) { $cmd = Get-Command dotnet -ErrorAction SilentlyContinue; if ($cmd) { $DOTNET_EXE = $cmd.Source } }
if ($DOTNET_EXE) {
    $env:Path = (Split-Path $DOTNET_EXE -Parent) + ';' + $env:Path
}

Write-Host '[Build Sharpmake]'
if (-not $DOTNET_EXE) {
    Write-Error 'dotnet.exe not found. Install .NET SDK from https://aka.ms/dotnet/download'
    exit 1
}
& $DOTNET_EXE build --configuration Release (Join-Path $PSScriptRoot 'Programs\Sharpmake\Sharpmake.Application\Sharpmake.Application.csproj')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Balius / Rust
Write-Host '[Build balius]'
$cargo = Get-Command cargo -ErrorAction SilentlyContinue
if (-not $cargo) {
    $rustup = Join-Path $env:TEMP 'rustup-init.exe'
    Invoke-WebRequest -Uri 'https://static.rust-lang.org/rustup/dist/x86_64-pc-windows-msvc/rustup-init.exe' -UseBasicParsing -OutFile $rustup
    Start-Process -FilePath $rustup -ArgumentList '-y', '--default-toolchain', 'nightly' -Wait -NoNewWindow
}
Push-Location (Join-Path $PSScriptRoot 'balius')
try {
    cmd /c 'rustup toolchain install 2>nul'
    $env:RUSTFLAGS = '-C debuginfo=0'
    & cargo b -r
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally { Pop-Location }

# Apply VS environment (vcvars64) to this process (helper batch avoids cmd line length limit)
$vcvars = Join-Path $VSPath 'VC\Auxiliary\Build\vcvars64.bat'
if (Test-Path $vcvars) {
    $helperBat = Join-Path $env:TEMP "vcvars_env_$PID.bat"
    "@echo off`ncall `"$vcvars`"`nset" | Set-Content -Path $helperBat -Encoding ASCII
    try {
        $lines = cmd /c "`"$helperBat`""
        $lines | ForEach-Object {
            if ($_ -match '^([^=]+)=(.*)$') {
                Set-Item -Path "env:$($matches[1])" -Value $matches[2] -ErrorAction SilentlyContinue
            }
        }
    } finally {
        if (Test-Path $helperBat) { Remove-Item $helperBat -Force }
    }
}

# Header-parser (CMake)
Write-Host '[Build header-parser]'
$hpDir = Join-Path $PSScriptRoot 'Programs\header-parser'
Push-Location $hpDir
try {
    if (Test-Path 'CMakeCache.txt') { Remove-Item 'CMakeCache.txt' -Force }
    if (Test-Path 'CMakeFiles') { Remove-Item 'CMakeFiles' -Recurse -Force }
    & cmake -G 'Visual Studio 18 2026' -A x64 -DCMAKE_BUILD_TYPE=Release .
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    & cmake --build . --config Release
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally { Pop-Location }

# VCPKG
Write-Host '[Run VCPKG]'
$vcpkgRoot = Join-Path $PSScriptRoot 'vcpkg_installed'
if (-not (Test-Path $vcpkgRoot)) { New-Item -ItemType Directory -Path $vcpkgRoot | Out-Null }
$ninjaDir = "$env:LocalAppData\vcpkg\downloads\tools\ninja\1.13.1-windows"
if (Test-Path $ninjaDir) { Remove-Item $ninjaDir -Recurse -Force -ErrorAction SilentlyContinue }
& vcpkg install --x-install-root (Join-Path $PSScriptRoot 'vcpkg_installed')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host 'Setup completed.'
