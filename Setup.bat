@echo off
:: BatchGotAdmin
:-------------------------------------
REM  --> Check for permissions
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"

REM --> If error flag set, we do not have admin.
if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    set params = %*:"=""
    echo UAC.ShellExecute "cmd.exe", "/c %~s0 %params%", "", "runas", 1 >> "%temp%\getadmin.vbs"

    "%temp%\getadmin.vbs"
    del "%temp%\getadmin.vbs"
    exit /B

:gotAdmin
    pushd "%CD%"
    CD /D "%~dp0"
:--------------------------------------

echo [Install Graphic Tools for debugging]
dism /online /add-capability /capabilityname:Tools.Graphics.DirectX~~~~0.0.1.0

:: Find Visual Studio using vswhere (supports any VS/Build Tools version and install path)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPath="
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSPath=%%i"
)
if not defined VSPath (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2026\BuildTools" set "VSPath=%ProgramFiles%\Microsoft Visual Studio\2026\BuildTools"
)
if not defined VSPath (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2026\Community" set "VSPath=%ProgramFiles%\Microsoft Visual Studio\2026\Community"
)
if not defined VSPath (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2026\Professional" set "VSPath=%ProgramFiles%\Microsoft Visual Studio\2026\Professional"
)
if not defined VSPath (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2026\Enterprise" set "VSPath=%ProgramFiles%\Microsoft Visual Studio\2026\Enterprise"
)
if not defined VSPath (
    if exist "%PROGRAMFILES(X86)%\Microsoft Visual Studio\2022\BuildTools" set "VSPath=%PROGRAMFILES(X86)%\Microsoft Visual Studio\2022\BuildTools"
)
if not defined VSPath (
    echo [No Visual Studio detected - Installing Visual Studio 2026 Build Tools]
    bitsadmin /transfer vsbuildtool /download /priority FOREGROUND "https://aka.ms/vs/stable/vs_buildtools.exe" "%TEMP%\vsbuildtool.exe"
    start /b /wait "" "%TEMP%/vsbuildtool.exe" "--passive" "--wait" "--add" "Microsoft.VisualStudio.Workload.VCTools;includeRecommended" "--add" "Microsoft.VisualStudio.Component.Windows11SDK.22621^" --add "Microsoft.VisualStudio.Component.VC.CMake.Project^" "--add" "Microsoft.VisualStudio.Workload.MSBuildTools;includeRecommended" "--add" "Microsoft.VisualStudio.Workload.ManagedDesktopBuildTools;includeRecommended" "--add" "Microsoft.NetCore.Component.Runtime.9.0^" "--add" "Microsoft.NetCore.Component.SDK^" "--add" "Microsoft.VisualStudio.Component.Vcpkg" "--add" "Microsoft.VisualStudio.Component.VC.CLI.Support^" "--add" "Microsoft.VisualStudio.Component.VC.ATLMFC^" "--addProductLang" "en-us"
    if exist "%ProgramFiles%\Microsoft Visual Studio\2026\BuildTools" set "VSPath=%ProgramFiles%\Microsoft Visual Studio\2026\BuildTools"
)
if not defined VSPath (
    echo No Visual Studio or Build Tools installation has been found.
    exit /b 1
)

:: Find dotnet.exe: script default location, then system locations, then PATH
:: (dotnet-install.ps1 default is %LocalAppData%\Microsoft\dotnet when -InstallDir is omitted)
set "DOTNET_EXE="
if exist "%LocalAppData%\Microsoft\dotnet\dotnet.exe" set "DOTNET_EXE=%LocalAppData%\Microsoft\dotnet\dotnet.exe"
if not defined DOTNET_EXE if exist "%ProgramW6432%\dotnet\dotnet.exe" set "DOTNET_EXE=%ProgramW6432%\dotnet\dotnet.exe"
if not defined DOTNET_EXE if exist "%ProgramFiles%\dotnet\dotnet.exe" set "DOTNET_EXE=%ProgramFiles%\dotnet\dotnet.exe"
if not defined DOTNET_EXE for /f "delims=" %%e in ('where dotnet 2^>nul') do if not defined DOTNET_EXE set "DOTNET_EXE=%%e"

:: Install .NET SDK if no dotnet with SDK found (no -InstallDir: script uses its own default)
set "HAVESDK="
if defined DOTNET_EXE for /f "delims=" %%i in ('"%DOTNET_EXE%" --list-sdks 2^>nul') do set "HAVESDK=1"
if not defined HAVESDK (
    echo [No .NET SDK found - Installing .NET 9.0 SDK]
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-WebRequest -Uri 'https://dot.net/v1/dotnet-install.ps1' -UseBasicParsing -OutFile '%TEMP%\dotnet-install.ps1'"
    if exist "%TEMP%\dotnet-install.ps1" (
        powershell -NoProfile -ExecutionPolicy Bypass -File "%TEMP%\dotnet-install.ps1" -Channel 9.0 -NoPath
    )
)

:: Re-find dotnet.exe after possible install
set "DOTNET_EXE="
if exist "%LocalAppData%\Microsoft\dotnet\dotnet.exe" set "DOTNET_EXE=%LocalAppData%\Microsoft\dotnet\dotnet.exe"
if not defined DOTNET_EXE if exist "%ProgramW6432%\dotnet\dotnet.exe" set "DOTNET_EXE=%ProgramW6432%\dotnet\dotnet.exe"
if not defined DOTNET_EXE if exist "%ProgramFiles%\dotnet\dotnet.exe" set "DOTNET_EXE=%ProgramFiles%\dotnet\dotnet.exe"
if not defined DOTNET_EXE for /f "delims=" %%e in ('where dotnet 2^>nul') do if not defined DOTNET_EXE set "DOTNET_EXE=%%e"

if defined DOTNET_EXE for %%d in ("%DOTNET_EXE%") do set "PATH=%%~dpd;%PATH%"

echo [Build Sharpmake]
if not defined DOTNET_EXE (
    echo ERROR: dotnet.exe not found. Install .NET SDK from https://aka.ms/dotnet/download
    exit /b 1
)
"%DOTNET_EXE%" build --configuration Release Programs\Sharpmake\Sharpmake.Application\Sharpmake.Application.csproj

echo [Build balius]
where cargo
IF errorlevel 1 (
bitsadmin /transfer rustdownload /download /priority FOREGROUND "https://static.rust-lang.org/rustup/dist/x86_64-pc-windows-msvc/rustup-init.exe" "%TEMP%\rustup-init.exe"
start /b /wait "" "%TEMP%/rustup-init.exe" "-y" "--default-toolchain" "nightly"
)

pushd balius
rustup toolchain install
set "RUSTFLAGS=-C debuginfo=0"
cargo b -r
popd

SET "VSDevEnv=%VSPath%\VC\Auxiliary\Build\vcvars64.bat"
call "%VSDevEnv%"

echo [Build header-parser]
pushd Programs\header-parser
if exist CMakeCache.txt del CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles
start /b /wait "" "cmake" "-G" "Visual Studio 18 2026" "-A" "x64" "."
start /b /wait "" "cmake" "--build" "." "--config" "Release"
popd

echo [Run VCPKG]
IF NOT EXIST vcpkg_installed (mkdir vcpkg_installed)
:: Remove stuck ninja tool dir so vcpkg can re-extract (fixes "extraction target already exists")
if exist "%LocalAppData%\vcpkg\downloads\tools\ninja\1.13.1-windows" rmdir /s /q "%LocalAppData%\vcpkg\downloads\tools\ninja\1.13.1-windows"
vcpkg install --x-install-root=%cd%\vcpkg_installed
