@echo off
set curpath=%~dp0

set X_ROOT=%cd%
set X_RES=%X_ROOT%/res/
set X_BIN=%X_ROOT%/bin/Debug

cd %curpath%

echo X_ROOT = %X_ROOT%
echo X_RES = %X_RES%
echo X_BIN = %X_BIN%


start %X_BIN%/robot.exe
