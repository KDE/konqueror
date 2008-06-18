#! /usr/bin/env bash
$EXTRACTRC *.ui >> rc.cpp || exit 11
$EXTRACTRC *.rc >> rc.cpp || exit 12
$XGETTEXT *.cpp -o $podir/webkitkde.pot
rm -f rc.cpp
