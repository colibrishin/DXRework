@echo off
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

SET Msbuild="%PROGRAMFILES%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
SET Cmake="cmake"
call vcvars64.bat

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
start /b /wait "" cmake "CMakeLists.txt"
start /b /wait "" cmake "--build" "."
popd

set VSVcpkg="%PROGRAMFILES%\Microsoft Visual Studio\2022\Community\VC\vcpkg\vcpkg.exe"
IF EXIST %VSVcpkg% (echo Found Visual studo vcpkg) else (echo No vcpkg from Visual studio found, fallback to env path)
IF EXIST %VSVcpkg% (%VSVcpkg% install) ELSE (vcpkg install)