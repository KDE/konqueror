project(konq-plugins)

#search packages used by KDE
set(KDE_MIN_VERSION "4.6.1")
set(SOPRANO_MIN_VERSION "2.5.63")
find_package(KDE4 4.6.1 REQUIRED)
find_package(Strigi REQUIRED)
find_package(ZLIB REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)

# include_directories (${KDEBASE_APPS_SOURCE_DIR}/lib/konq)
include_directories ( ${KDE4_INCLUDES})

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

