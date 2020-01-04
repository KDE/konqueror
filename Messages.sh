#! /usr/bin/env bash
$EXTRACTRC `find src -name \*.rc` >> rc.cpp || exit 11
$EXTRACTRC `find src -name \*.ui` >> rc.cpp || exit 12
$EXTRACTRC `find src -name \*.kcfg` >> rc.cpp
$XGETTEXT -kaliasLocal `find src -name \*.cc -o -name \*.cpp -o -name \*.h` rc.cpp -o $podir/konqueror.pot
rm -f rc.cpp
