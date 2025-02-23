$EXTRACTRC `find . -name \*.rc -o -name \*.ui` >> rc.cpp || exit 1
# Exclude sidebar/trees/ which still references Qt3
# and it needs to be either ported or deleted.
$XGETTEXT `find . \( -name \*.cpp -o -name \*.h \) -not -path "./trees/*"` -o $podir/konqsidebar.pot
rm -f rc.cpp
