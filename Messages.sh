#! /usr/bin/env bash
$EXTRACTRC *.ui */*.ui >> rc.cpp || exit 11
$EXTRACTRC *.rc */*.rc >> rc.cpp || exit 12
$XGETTEXT *.cpp */*.cpp -o $podir/webkitkde.pot
rm -f rc.cpp
