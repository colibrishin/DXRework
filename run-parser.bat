@echo off
rem [EngineDir] [ProjectName] [ProjectDir] [WINDIR] [GITPATH]

echo Given project directory is %2
echo Given root directory is %1
cd /d %1

if not exist Intermediate\HeaderParser\ (
echo Intermediate header directory not found... Generating...
mkdir Intermediate\HeaderParser
mkdir Intermediate\HeaderParser\HeaderGenerated
)

echo Check other instances...
:WAIT_LOCK
if exist (lock) do goto WAIT_LOCK
rem todo: PID를 저장하고 진행중인 배치파일이 살아있는지 한번 더 확인
type nul > lock

if not exist Intermediate\HeaderParser\.git (
echo Header tracking git repository is not initialized... Initializing...
pushd Intermediate\HeaderParser
echo target >> .gitignore
echo HeaderGenerated/*** >> .gitignore
%5\bin\sh.exe --login -c "git init && git add . && git commit -m "Initialization""
popd
)

pushd Intermediate\HeaderParser
echo Copying header from %3...
if not exist %2 (mkdir %2)
%4\System32\robocopy %3 %2 "*.h??" /xo /nodcopy /s

echo Comparing header changes...
%5\bin\sh.exe --login -c "git status --porcelain -uall | cut -c 1-3 --complement | egrep .hp?p?$ > target && git add . && git commit -m "header update""
echo Parsing header of %2...
for /f "tokens=*" %%a in (target) do (
  "../../Programs/header-parser/Release/header-parser.exe" %%a -c ECLASS -e EENUM -f EFUNC -p EPROPERTY
)
popd

del lock