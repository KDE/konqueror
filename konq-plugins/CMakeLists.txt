project(konq-plugins)

include_directories (${KDE_BASEAPPS_SOURCE_DIR}/lib/konq)

# TODO add_subdirectory( khtmlsettingsplugin )
# TODO add_subdirectory( kimgalleryplugin )
# TODO add_subdirectory( dirfilter )
# TODO add_subdirectory( uachanger )
# TODO add_subdirectory( babelfish )
# TODO add_subdirectory( validators )
# TODO add_subdirectory( domtreeviewer )
# TODO add_subdirectory( webarchiver )

#add_subdirectory( quickprint )
# TODO add_subdirectory( kttsplugin )

if(UNIX)
   # TODO add_subdirectory( shellcmdplugin )
endif(UNIX)

# TODO add_subdirectory( imagerotation )
# TODO add_subdirectory( minitools )
#add_subdirectory( microformat )
# TODO add_subdirectory( autorefresh )
add_subdirectory( fsview )
# TODO add_subdirectory( searchbar )
# TODO add_subdirectory( rellinks )

# TODO add_subdirectory( adblock )

# TODO add_subdirectory( akregator )

