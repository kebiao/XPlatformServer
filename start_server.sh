#!/bin/sh

export X_ROOT=$(pwd)
export X_RES="$X_ROOT/res/"
export X_BIN="$X_ROOT/bin"

echo X_ROOT = \"${X_ROOT}\"
echo X_RES = \"${X_RES}\"
echo X_BIN = \"${X_BIN}\"

sh ./kill_server.sh

$X_BIN/directory&
$X_BIN/machine&
$X_BIN/dbmgr&
$X_BIN/halls&
$X_BIN/hallsmgr&
$X_BIN/roommgr&
$X_BIN/connector&
$X_BIN/login&


