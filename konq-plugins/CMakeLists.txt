project(konq-plugins)

include_directories (${KDEBASE_APPS_SOURCE_DIR}/lib/konq)

add_subdirectory( khtmlsettingsplugin )
add_subdirectory( kimgalleryplugin )
add_subdirectory( dirfilter )
add_subdirectory( uachanger )
add_subdirectory( babelfish )
add_subdirectory( validators )
add_subdirectory( domtreeviewer )
add_subdirectory( webarchiver )

add_subdirectory( imagerotation )
add_subdirectory( minitools )
#add_subdirectory( microformat )
add_subdirectory( autorefresh )
add_subdirectory( fsview )
add_subdirectory( searchbar )
add_subdirectory( rellinks )

add_subdirectory(adblock)

add_subdirectory( akregator )

