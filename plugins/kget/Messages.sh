#! /bin/sh
$EXTRACTRC *.rc >> rc.cpp || exit 11
$XGETTEXT *.cpp -o $podir/kgetplugin.pot
rm -f rc.cpp
