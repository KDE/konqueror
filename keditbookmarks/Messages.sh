#! /usr/bin/env bash
$XGETTEXT -kaliasLocal `find . -name \*.cc -o -name \*.cpp -o -name \*.h` -o $podir/keditbookmarks.pot
