title HeaderParserBatchFile
@echo off
rem [EngineDir] [ProjectName] [ProjectDir] [WINDIR] [GITPATH]

if %1=="" (goto EXIT)
if "%2"=="" (goto EXIT)
if %3=="" (goto EXIT)
if %4=="" (goto EXIT)
if %5=="" (goto EXIT)

set "XCOPY_PATH=%4\System32\xcopy.exe"
set "GITPATH=%5"
set "SH_PATH=%GITPATH:"=%\bin\sh.exe"

echo Given project directory is %2
echo Given root directory is %1
cd /d %1

if not exist Intermediate\HeaderParser\ (
echo Intermediate header directory not found... Generating...
mkdir Intermediate\HeaderParser
mkdir Intermediate\HeaderParser\HeaderGenerated
)

:WAIT_LOCK
echo Check other instances...
if not exist lock (goto CRITICAL) else (goto WAIT_LOCK)

:CRITICAL
tasklist /V /FI "WindowTitle eq HeaderParserBatchFile*" /FO csv /nh  > lock

if not exist Intermediate\HeaderParser\.git (
echo Header tracking git repository is not initialized... Initializing...
pushd Intermediate\HeaderParser
echo target >> .gitignore
echo HeaderGenerated/*** >> .gitignore
start "" /b /wait "%SH_PATH%" -c "git init && git add . && git commit -m Init"
popd
)

pushd Intermediate\HeaderParser
echo Copying header from %3...
if not exist %2 (mkdir %2)
for /r %3 %%f in (*.h) do %XCOPY_PATH% %%f %2 /y /d

echo Comparing header changes...
set "COMMAND=git status --porcelain -uall | cut -c 1-3 --complement | egrep .h$ > target && git add . && git commit -m header-update"
start "" /b /wait "%SH_PATH%" -c "%COMMAND%"
echo Parsing header of %2...
for /f "tokens=*" %%a in (target) do (
  start "" /b /wait "..\..\Programs\header-parser\Release\header-parser.exe" %%a -c ECLASS -e EENUM -f EFUNC -p EPROPERTY 2>&1
)
popd
del lock
exit /b

:EXIT
echo Insufficient argument given.
exit /b