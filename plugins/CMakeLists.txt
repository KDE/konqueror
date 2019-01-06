
find_package(Qt5TextToSpeech ${QT_REQUIRED_VERSION} QUIET)
set_package_properties(Qt5TextToSpeech PROPERTIES
      PURPOSE "Gives Konqueror a plugin to speak portions or all of a website")

add_subdirectory( khtmlsettingsplugin )
add_subdirectory( kimgalleryplugin )
add_subdirectory( dirfilter )
# TODO add_subdirectory( uachanger )
add_subdirectory( babelfish )
add_subdirectory( validators )
add_subdirectory( domtreeviewer )
add_subdirectory( webarchiver )

if (Qt5TextToSpeech_FOUND)
   add_subdirectory(ttsplugin)
endif()

if(UNIX)
   add_subdirectory( shellcmdplugin )
endif(UNIX)

# TODO add_subdirectory( imagerotation )
add_subdirectory( minitools )
#add_subdirectory( microformat )
add_subdirectory( autorefresh )
if(UNIX)
   add_subdirectory( fsview )
endif()
add_subdirectory( searchbar )
add_subdirectory( rellinks )

# TODO add_subdirectory( adblock )

add_subdirectory( akregator )

