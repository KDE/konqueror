#!/bin/sh
#
# Kdelibs coding style is defined in http://techbase.kde.org/Policies/Kdelibs_Coding_Style 
#

PWD=$(pwd)
cd $PWD

echo "Applying astyle rules..."
astyle -v --indent=spaces=4 \
       --brackets=linux \
       --indent-labels \
       --pad=oper --unpad=paren \
       --one-line=keep-statements \
       --convert-tabs --indent-preprocessor \
       `find -type f -name '*.cpp' -or -name '*.h' -or -name '*.cc' | grep -Ev "\./.+/settings/"`

echo "Done!"

