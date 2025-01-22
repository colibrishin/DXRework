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
start /b /wait "" %Cmake% "--build" "."
popd

echo [Run VCPKG]
pushd Progams\vcpkg
IF NOT EXIST Programs\vcpkg (
git clone https://github.com/Microsoft/vcpkg.git
call bootstrap-vcpkg.bat
popd
)

set "VcpkgPath=Programs\vcpkg\vcpkg.exe"
IF NOT EXIST vcpkg_installed (mkdir vcpkg_installed)
IF EXIST "%VcpkgPath%" ("%VcpkgPath%" install --x-install-root=%cd%\vcpkg_installed)