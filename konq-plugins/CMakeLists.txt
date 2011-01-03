project(konq-plugins)

# search packages used by KDE
find_package(KDE4 REQUIRED)
find_package(LibKonq REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -DQT_STRICT_ITERATORS)
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

if( NOT LIBKONQ_FOUND )
    message(FATAL_ERROR "Install kdebase before to try to compile konq-plugins")
endif( NOT LIBKONQ_FOUND )

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

macro_display_feature_log()

