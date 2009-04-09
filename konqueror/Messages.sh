#! /usr/bin/env bash
subdirs="src sidebar about shellcmdplugin"
$EXTRACTRC `find $subdirs -name \*.rc` >> rc.cpp || exit 11
$EXTRACTRC `find $subdirs -name \*.ui` >> rc.cpp || exit 12
$EXTRACTRC `find $subdirs -name \*.kcfg` >> rc.cpp
$XGETTEXT -kaliasLocal `find $subdirs -name \*.cc -o -name \*.cpp -o -name \*.h` rc.cpp -o $podir/konqueror.pot
rm -f rc.cpp
