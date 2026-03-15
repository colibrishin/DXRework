@echo off
:: Request admin then run Setup.ps1 in the same window (no new PowerShell window).
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    echo UAC.ShellExecute "cmd.exe", "/c " ^& Chr^(34^) ^& "%~s0" ^& Chr^(34^), "", "runas", 1 >> "%temp%\getadmin.vbs"
    "%temp%\getadmin.vbs"
    del "%temp%\getadmin.vbs"
    exit /B
)
pushd "%CD%"
CD /D "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Setup.ps1"
set "EXITCODE=%errorlevel%"
popd
exit /B %EXITCODE%
