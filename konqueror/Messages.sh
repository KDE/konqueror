#! /usr/bin/env bash
subdirs="src sidebar about kttsplugin remoteencodingplugin shellcmdplugin"
$EXTRACTRC `find $subdirs -name \*.rc` >> rc.cpp || exit 11
$EXTRACTRC `find $subdirs -name \*.ui` >> rc.cpp || exit 12
$XGETTEXT -kaliasLocal `find $subdirs -name \*.cc -o -name \*.cpp -o -name \*.h` rc.cpp -o $podir/konqueror.pot
