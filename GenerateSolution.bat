@echo off

REM EngineDir은 EngineSolution.build.cs에서 사용 됩니다.
set EngineDir=%cd%
IF NOT "%~1"=="" set "EngineDir=%~1"
echo EngineDir: %EngineDir%

REM TargetCS는 build에 진입점이 될 cs파일의 경로입니다.
set TargetCS=%EngineDir%\Engine\Source\EngineMain.build.cs
IF NOT "%~2"=="" set TargetCS="%2"
echo TargetCS: %TargetCS%

set FrontendTargetCS=%EngineDir%\Engine\KolibriMain.build.cs
IF NOT "%~3"=="" set FrontendTargetCS="%3"
echo FrontendTargetCS: %FrontendTargetCS%

REM SharpMakeSolutionDir은 최종 Sln 파일이 만들어질 위치가 됩니다.
REM SharpMakeSolutionDir은 EngineSolution.build.cs에서 사용 됩니다.
set SharpMakeSolutionDir=%cd%
IF NOT "%~4"=="" set SharpMakeSolutionDir="%4"
echo SharpMakeSolutionDir: %SharpMakeSolutionDir%

set VSVcpkg="%PROGRAMFILES%\Microsoft Visual Studio\2022\Community\VC\vcpkg\vcpkg.exe"
IF EXIST %VSVcpkg% (echo "Found Visual studo vcpkg") else (echo "No vcpkg from Visual studio found, fallback to env path")
IF EXIST %VSVcpkg% (%VSVcpkg% install) ELSE (vcpkg install)

"%EngineDir%\Programs\Sharpmake\Sharpmake.Application\bin\Release\net6.0\Sharpmake.Application.exe" /sources(@'%TargetCS%') /verbose
"%EngineDir%\Programs\Sharpmake\Sharpmake.Application\bin\Release\net6.0\Sharpmake.Application.exe" /sources(@'%FrontendTargetCS%') /verbose