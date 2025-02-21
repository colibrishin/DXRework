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

REM echo [Install latest build tool]
REM bitsadmin /transfer vsbuildtool /download /priority FOREGROUND "https://aka.ms/vs/17/release/vs_BuildTools.exe" "%TEMP%\vsbuildtool.exe"
REM start /b /wait "" "%TEMP%/vsbuildtool.exe" "--passive" "--wait" "--add" "Microsoft.VisualStudio.Workload.VCTools;includeRecommended" "--add" "Microsoft.VisualStudio.Component.Windows11SDK.22621^" --add "Microsoft.VisualStudio.Component.VC.CMake.Project^" "--add" "Microsoft.VisualStudio.Workload.MSBuildTools;includeRecommended"

echo [Build Sharpmake]
dotnet build --configuration Release Programs\Sharpmake\Sharpmake.Application\Sharpmake.Application.csproj

echo [Build balius]
where cargo
IF errorlevel 1 (
bitsadmin /transfer rustdownload /download /priority FOREGROUND "https://static.rust-lang.org/rustup/dist/x86_64-pc-windows-msvc/rustup-init.exe" "%TEMP%\rustup-init.exe"
start /b /wait "" "%TEMP%/rustup-init.exe" "-y"
) ELSE (
pushd balius
cargo b -r
popd)

IF EXIST "%PROGRAMFILES%\Microsoft Visual Studio\2022\Community" (
    SET "VSPath=%PROGRAMFILES%\Microsoft Visual Studio\2022\Community"
) else if EXIST "%PROGRAMFILES%\Microsoft Visual Studio\2022\Professional" (
    SET "VSPath=%PROGRAMFILES%\Microsoft Visual Studio\2022\Professional"
) else if EXIST "%PROGRAMFILES%\Microsoft Visual Studio\2022\Enterprise" (
    SET "VSPath=%PROGRAMFILES%\Microsoft Visual Studio\2022\Enterprise"
) else (
    echo "No Visual Studio has been found"
    exit /b
)

SET Cmake="cmake"
SET "VSDevEnv=%VSPath%\VC\Auxiliary\Build\vcvars64.bat"
call "%VSDevEnv%"

where cmake
IF errorlevel 1 (
echo [Download CMake]
IF NOT EXIST Programs\CMake (
bitsadmin /transfer cmakedownload /download /priority FOREGROUND "https://github.com/Kitware/CMake/releases/download/v3.31.4/cmake-3.31.4-windows-x86_64.zip" "%TEMP%\cmake.zip"
powershell Expand-Archive %TEMP%\cmake.zip -DestinationPath Programs\CMake)
SET Cmake="..\CMake\cmake-3.31.4-windows-x86_64\bin\cmake.exe"
)

echo [Build header-parser]
pushd Programs\header-parser
start /b /wait "" %Cmake% "CMakeLists.txt"
start /b /wait "" %Cmake% "--build" "." "--config" "Release"
popd

echo [Run VCPKG]
IF NOT EXIST Programs\vcpkg (
pushd Programs
git clone https://github.com/Microsoft/vcpkg.git
popd
)

IF NOT EXIST Programs\vcpkg\vcpkg.exe (
pushd Programs\vcpkg
call bootstrap-vcpkg.bat
popd
)

set "VcpkgPath=Programs\vcpkg\vcpkg.exe"
IF NOT EXIST vcpkg_installed (mkdir vcpkg_installed)
IF EXIST "%VcpkgPath%" ("%VcpkgPath%" install --x-install-root=%cd%\vcpkg_installed)