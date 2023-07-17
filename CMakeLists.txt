cmake_minimum_required(VERSION 3.5)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "23")
set (RELEASE_SERVICE_VERSION_MINOR "07")
set (RELEASE_SERVICE_VERSION_MICRO "80")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(Konqueror VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "5.15.2")
set(KF5_MIN_VERSION "5.101.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82.0")

find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)

include(FeatureSummary)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

set(KONQUEROR_LIB_VERSION "${RELEASE_SERVICE_VERSION}")
set(KONQUEROR_VERSION "${KONQUEROR_LIB_VERSION}")

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Widgets WebEngineWidgets)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Parts KCMUtils Archive Crash WindowSystem Init IconThemes DBusAddons GuiAddons)

find_package(KF5 ${KF5_MIN_VERSION} COMPONENTS Activities DocTools) # Optional

option(WITHOUT_X11 "Build without X11 integration (skips finding Qt5X11Extras)" OFF)
if(NOT WITHOUT_X11)
    find_package(Qt5X11Extras)
    set(KONQ_HAVE_X11 ${Qt5X11Extras_FOUND})
endif()

add_definitions(
    -DQT_USE_QSTRINGBUILDER
)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

include (CheckSymbolExists)
check_symbol_exists(mallinfo        "malloc.h"                 KDE_MALLINFO_MALLOC)
check_symbol_exists(mallinfo        "stdlib.h"                 KDE_MALLINFO_STDLIB)
# TODO KDE_MALLINFO_FIELD_hblkhd
# TODO KDE_MALLINFO_FIELD_uordblks
# TODO KDE_MALLINFO_FIELD_usmblks
configure_file (config-konqueror.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konqueror.h )

configure_file (konqueror-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/konqueror-version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory( libkonq )
add_subdirectory( src )
add_subdirectory( client )
if(BUILD_TESTING)
    add_subdirectory( autotests )
endif()
add_subdirectory( webenginepart )

add_subdirectory( pics )
add_subdirectory( sidebar )
add_subdirectory( settings )
add_subdirectory( plugins )

if (KF5DocTools_FOUND)
    add_subdirectory( doc )
endif()

set(DEVELOPER_MODE false CACHE BOOL "Enables developer mode, which allows running separate instances of Konqueror")

########### install files ###############
install( PROGRAMS
   kfmclient.desktop kfmclient_html.desktop kfmclient_war.desktop kfmclient_dir.desktop
   konqbrowser.desktop
   DESTINATION  ${KDE_INSTALL_APPDIR} )
install(FILES org.kde.konqueror.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

# Make sure kfmclient can find konqueror.desktop even if people remove it from the K menu (#62242)
# This is why konqueror.desktop is installed into services.
install( FILES org.kde.konqueror.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
# On Wayland the icon to use for the main window is read from the desktop file, which needs
# to be in the applications directory, so it needs to be installed there, too (bug #452149)
install( FILES org.kde.konqueror.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

install(FILES konqy_preload.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

install(FILES konqueror.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

ki18n_install(po)
if (KF5DocTools_FOUND)
    kdoctools_install(po)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
