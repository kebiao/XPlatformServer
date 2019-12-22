@echo off
set curpath=%~dp0

set X_ROOT=%cd%
set X_RES=%X_ROOT%/res/
set X_BIN=%X_ROOT%/bin/Debug

cd %curpath%
call "kill_server.bat"

echo X_ROOT = %X_ROOT%
echo X_RES = %X_RES%
echo X_BIN = %X_BIN%

start %X_BIN%/directory.exe
start %X_BIN%/machine.exe
start %X_BIN%/dbmgr.exe
start %X_BIN%/login.exe
start %X_BIN%/halls.exe
start %X_BIN%/hallsmgr.exe
start %X_BIN%/roommgr.exe
start %X_BIN%/connector.exe
