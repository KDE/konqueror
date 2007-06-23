#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.rc` >> rc.cpp || exit 11
$XGETTEXT -kaliasLocal `find . -name \*.cc -o -name \*.cpp -o -name \*.h` -o $podir/keditbookmarks.pot
