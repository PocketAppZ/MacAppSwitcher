@echo off
REM https://superuser.com/questions/788924/is-it-possible-to-automatically-run-a-batch-file-as-administrator

>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"

if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    set params = %*:"=""
    echo UAC.ShellExecute "cmd.exe", "/c %~s0 %params%", "", "runas", 1 >> "%temp%\getadmin.vbs"

    "%temp%\getadmin.vbs"
    del "%temp%\getadmin.vbs"
    exit /B

:gotAdmin
    pushd "%CD%"
    CD /D "%~dp0"

set "fullPath=%cd%\AltAppSwitcher.exe"
echo %fullpath%
schtasks /create /sc ONEVENT /ec Application /tn AltAppSwitcher /tr %fullPath% /RL HIGHEST /F

reg add "HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" /v "AltAppSwitcher" /t REG_SZ /d "schtasks /run /tn AltAppSwitcher" /f

if ERRORLEVEL == 0 (
    msg * "AltAppSwitcher has been added to startup apps. Please re-run this utility if you move the application executable."
    exit
)

    pause