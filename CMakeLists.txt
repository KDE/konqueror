cmake_minimum_required(VERSION 3.16)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
include(CMakeDependentOption)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "03")
set (RELEASE_SERVICE_VERSION_MICRO "90")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(Konqueror VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "6.4.0")
set(KF5_MIN_VERSION "5.240.0")


set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})
find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)


set(KF_MAJOR_VERSION "6")
include(KDECompilerSettings NO_POLICY_SCOPE)
remove_definitions(-DQT_NO_KEYWORDS)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

include(FeatureSummary)

set(KONQUEROR_LIB_VERSION "${RELEASE_SERVICE_VERSION}")
set(KONQUEROR_VERSION "${KONQUEROR_LIB_VERSION}")

find_package(Qt${KF_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Widgets WebEngineWidgets)
find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS Parts KCMUtils Archive Crash WindowSystem IconThemes DBusAddons GuiAddons I18n Sonnet TextWidgets Codecs)

find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} COMPONENTS DocTools) # Optional
find_package(PlasmaActivities ${KF_MIN_VERSION}) # Optional
set(KActivities_FOUND ${PlasmaActivities_FOUND})

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
add_subdirectory(libkonqsettings)
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
add_subdirectory( kioworkers )

if (KF${KF_MAJOR_VERSION}DocTools_FOUND)
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
#TODO KF6: check whether the above comment is still relevant (remove Konqueror from the K menu and check
#whether kfmclient works correctly. If it doesn't, find out how to make it work in KF6
# if (KF_MAJOR_VERSION STRLESS "6")
#     install( FILES org.kde.konqueror.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
# endif()

# On Wayland the icon to use for the main window is read from the desktop file, which needs
# to be in the applications directory, so it needs to be installed there, too (bug #452149)
install( FILES org.kde.konqueror.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

install(FILES konqy_preload.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

install(FILES konqueror.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

ki18n_install(po)
if (KF${KF_MAJOR_VERSION}DocTools_FOUND)
    kdoctools_install(po)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

