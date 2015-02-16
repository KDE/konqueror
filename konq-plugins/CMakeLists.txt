cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
project(konq-plugins)

find_package(ECM 1.3.0 CONFIG REQUIRED)

include(FeatureSummary)

include_directories (${KDE_BASEAPPS_SOURCE_DIR}/lib/konq)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMInstallIcons)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(KF5 REQUIRED COMPONENTS KDELibs4Support Crash)

add_subdirectory( khtmlsettingsplugin )
add_subdirectory( kimgalleryplugin )
add_subdirectory( dirfilter )
# TODO add_subdirectory( uachanger )
add_subdirectory( babelfish )
add_subdirectory( validators )
add_subdirectory( domtreeviewer )
add_subdirectory( webarchiver )

#add_subdirectory( quickprint )
# TODO add_subdirectory( kttsplugin )

if(UNIX)
   add_subdirectory( shellcmdplugin )
endif(UNIX)

# TODO add_subdirectory( imagerotation )
add_subdirectory( minitools )
#add_subdirectory( microformat )
add_subdirectory( autorefresh )
add_subdirectory( fsview )
# TODO add_subdirectory( searchbar )
add_subdirectory( rellinks )

# TODO add_subdirectory( adblock )

add_subdirectory( akregator )

