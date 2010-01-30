#! /usr/bin/env bash
$EXTRACTRC `find . -name '*.rc' -or -name '*.ui'` >> rc.cpp || exit 11
$XGETTEXT `find . -name '*.cpp'` -o $podir/kwebkitpart.pot
rm -f rc.cpp
