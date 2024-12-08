@echo off
echo [Build Sharpmake]
dotnet build --configuration Release Programs\Sharpmake\Sharpmake.Application\Sharpmake.Application.csproj

set VSVcpkg="%PROGRAMFILES%\Microsoft Visual Studio\2022\Community\VC\vcpkg\vcpkg.exe"
IF EXIST %VSVcpkg% (echo Found Visual studo vcpkg) else (echo No vcpkg from Visual studio found, fallback to env path)
IF EXIST %VSVcpkg% (%VSVcpkg% install) ELSE (vcpkg install)