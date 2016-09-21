cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
project(konq-plugins)

find_package(ECM 1.7.0 CONFIG REQUIRED)

include(FeatureSummary)

include_directories (${KDE_BASEAPPS_SOURCE_DIR}/lib/konq)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMMarkAsTest)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt5TextToSpeech ${QT_REQUIRED_VERSION} QUIET)
set_package_properties(Qt5TextToSpeech PROPERTIES
      PURPOSE "Gives Konqueror a plugin to speak portions or all of a website")
find_package(KF5 REQUIRED COMPONENTS KDELibs4Support Crash)

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
add_subdirectory( fsview )
add_subdirectory( searchbar )
add_subdirectory( rellinks )

# TODO add_subdirectory( adblock )

add_subdirectory( akregator )

