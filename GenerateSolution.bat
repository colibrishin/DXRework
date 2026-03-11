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

"!SharpmakeDir!" /sources(@'%TargetCS%') /verbose
IF EXIST "%ClientCS%" "!SharpmakeDir!" /sources(@'%ClientCS%') /verbose
"!SharpmakeDir!" /sources(@'%FrontendTargetCS%') /verbose