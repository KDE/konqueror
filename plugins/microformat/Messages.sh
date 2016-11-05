#! /bin/sh
$EXTRACTRC *.rc >> rc.cpp
$XGETTEXT *.cpp -o $podir/mf_konqplugin.pot
