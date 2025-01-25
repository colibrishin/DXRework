@echo off

set EngineDir=%cd%
IF NOT "%~1"=="" set "EngineDir=%~1"
echo EngineDir: %EngineDir%

set TargetCS=%EngineDir%\Engine\Source\EngineMain.build.cs
IF NOT "%~2"=="" set TargetCS="%2"
echo TargetCS: %TargetCS%

set FrontendTargetCS=%EngineDir%\Engine\KolibriMain.build.cs
IF NOT "%~3"=="" set FrontendTargetCS="%3"
echo FrontendTargetCS: %FrontendTargetCS%

set SharpMakeSolutionDir=%cd%
IF NOT "%~4"=="" set SharpMakeSolutionDir="%~4"
echo SharpMakeSolutionDir: %SharpMakeSolutionDir%

IF EXIST "%EngineDir%\Programs\Sharpmake\Sharpmake.Application\bin\Release\net6.0\Sharpmake.Application.exe" (
SET SharpmakeDir="%EngineDir%\Programs\Sharpmake\Sharpmake.Application\bin\Release\net6.0\Sharpmake.Application.exe") ELSE IF EXIST "%EngineDir%\Programs\Sharpmake\Sharpmake.Application\bin\x64\Release\net6.0\Sharpmake.Application.exe" (
SET SharpmakeDir="%EngineDir%\Programs\Sharpmake\Sharpmake.Application\bin\x64\Release\net6.0\Sharpmake.Application.exe") ELSE (
echo Unable to find the sharpmake
exit /b)
echo SharpmakeDir: %SharpmakeDir%

"%SharpmakeDir%" /sources(@'%TargetCS%') /verbose
"%SharpmakeDir%" /sources(@'%FrontendTargetCS%') /verbose