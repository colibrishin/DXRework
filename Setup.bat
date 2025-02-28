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

echo [Install latest build tool]
bitsadmin /transfer vsbuildtool /download /priority FOREGROUND "https://aka.ms/vs/17/release/vs_BuildTools.exe" "%TEMP%\vsbuildtool.exe"
start /b /wait "" "%TEMP%/vsbuildtool.exe" "--passive" "--wait" "--add" "Microsoft.VisualStudio.Workload.VCTools;includeRecommended" "--add" "Microsoft.VisualStudio.Component.Windows11SDK.22621^" --add "Microsoft.VisualStudio.Component.VC.CMake.Project^" "--add" "Microsoft.VisualStudio.Workload.MSBuildTools;includeRecommended" "--add" "Microsoft.NetCore.Component.Runtime.9.0^" "--add" "Microsoft.VisualStudio.Component.Vcpkg" "--add" "Microsoft.VisualStudio.Component.VC.CLI.Support^" "--add" "Microsoft.VisualStudio.Component.VC.ATLMFC^" "--addProductLang" "en-us"

echo [Build Sharpmake]
dotnet build --configuration Release Programs\Sharpmake\Sharpmake.Application\Sharpmake.Application.csproj

echo [Build balius]
where cargo
IF errorlevel 1 (
bitsadmin /transfer rustdownload /download /priority FOREGROUND "https://static.rust-lang.org/rustup/dist/x86_64-pc-windows-msvc/rustup-init.exe" "%TEMP%\rustup-init.exe"
start /b /wait "" "%TEMP%/rustup-init.exe" "-y" "--default-toolchain" "nightly"
)

pushd balius
rustup toolchain install nightly
cargo +nightly b -r
popd

IF EXIST "%PROGRAMFILES(X86)%\Microsoft Visual Studio\2022\BuildTools" (
SET "VSPath=%PROGRAMFILES(X86)%\Microsoft Visual Studio\2022\BuildTools"
) else (
echo No Visual Studio Build tools has been found
exit /b
)

SET "VSDevEnv=%VSPath%\VC\Auxiliary\Build\vcvars64.bat"
call "%VSDevEnv%"

echo [Build header-parser]
pushd Programs\header-parser
start /b /wait "" "cmake" "CMakeLists.txt"
start /b /wait "" "cmake" "--build" "." "--config" "Release"
popd

echo [Run VCPKG]
IF NOT EXIST vcpkg_installed (mkdir vcpkg_installed)
vcpkg install --x-install-root=%cd%\vcpkg_installed