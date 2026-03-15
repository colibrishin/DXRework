@echo off
setlocal enabledelayedexpansion

set SHARPMAKE_DOTNET_VERSION=net8.0
set EngineDir=%cd%
IF NOT "%~1"=="" set "EngineDir=%~1"
echo EngineDir: %EngineDir%

set ClientCS=%EngineDir%\Client\ClientMain.build.cs
IF NOT "%~2"=="" set ClientCS="%2"
echo ClientCS: %ClientCS%

set TargetCS=%EngineDir%\Engine\Source\EngineMain.build.cs
IF NOT "%~2"=="" set TargetCS="%2"
echo TargetCS: %TargetCS%

set FrontendTargetCS=%EngineDir%\Engine\KolibriMain.build.cs
IF NOT "%~3"=="" set FrontendTargetCS="%3"
echo FrontendTargetCS: %FrontendTargetCS%

set SharpMakeSolutionDir=%cd%
IF NOT "%~4"=="" set SharpMakeSolutionDir="%~4"
echo SharpMakeSolutionDir: %SharpMakeSolutionDir%

set "SHARPMAKE_APP_PATH=%EngineDir%\Programs\Sharpmake\Sharpmake.Application\bin\Release\%SHARPMAKE_DOTNET_VERSION%\Sharpmake.Application.exe"
set "SHARPMAKE_APP_PATH_X64=%EngineDir%\Programs\Sharpmake\Sharpmake.Application\bin\x64\Release\%SHARPMAKE_DOTNET_VERSION%\Sharpmake.Application.exe"
set "SHARPMAKE_APP_CSPROJ=%EngineDir%\Programs\Sharpmake\Sharpmake.Application\Sharpmake.Application.csproj"

:: Rebuild Sharpmake before generating so generation uses the latest code.
echo [Build Sharpmake]
dotnet build "!SHARPMAKE_APP_CSPROJ!" -c Release -v q
if !errorlevel! neq 0 (
  echo Sharpmake build failed.
  exit /b !errorlevel!
)

:: Use dir instead of if exist (more reliable on some drives e.g. I:)
dir /b "!SHARPMAKE_APP_PATH!" >nul 2>&1
if !errorlevel! equ 0 (
  set "SharpmakeDir=!SHARPMAKE_APP_PATH!"
) else (
  dir /b "!SHARPMAKE_APP_PATH_X64!" >nul 2>&1
  if !errorlevel! equ 0 (
    set "SharpmakeDir=!SHARPMAKE_APP_PATH_X64!"
  ) else (
    echo Unable to find the sharpmake (expected %SHARPMAKE_DOTNET_VERSION%)
    echo Tried: !SHARPMAKE_APP_PATH!
    exit /b 1
  )
)
echo SharpmakeDir: !SharpmakeDir!

:: Generate header-parser.vcxproj so Sharpmake can include it in the solution (optional; skip if not present)
set "HEADERPARSER_VCXPROJ=%SharpMakeSolutionDir%\Programs\header-parser\header-parser.vcxproj"
if not exist "!HEADERPARSER_VCXPROJ!" (
  set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
  set "VSPath="
  if exist "!VSWHERE!" for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSPath=%%i"
  if not "!VSPath!"=="" (
    echo [Generate header-parser vcxproj for solution]
    call "!VSPath!\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    pushd "!SharpMakeSolutionDir!\Programs\header-parser"
    if exist CMakeLists.txt (
      if exist CMakeCache.txt del CMakeCache.txt
      if exist CMakeFiles rmdir /s /q CMakeFiles
      cmake -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release . >nul 2>&1
    )
    popd
  )
)

"!SharpmakeDir!" /sources(@'%TargetCS%') /verbose
IF EXIST "%ClientCS%" "!SharpmakeDir!" /sources(@'%ClientCS%') /verbose
"!SharpmakeDir!" /sources(@'%FrontendTargetCS%') /verbose