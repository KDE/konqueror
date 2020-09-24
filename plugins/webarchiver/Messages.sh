#! /bin/sh
$EXTRACTRC *.rc app/*.kcfg >> rc.cpp
$XGETTEXT *.cpp */*.cpp -o $podir/webarchiver.pot
