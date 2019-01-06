cmake_minimum_required(VERSION 3.0)

project(Konqueror)

set(QT_MIN_VERSION "5.5.0")
set(KF5_MIN_VERSION "5.31.0")  # needs KIO 5.31 for KRun::runUrl overload with RunFlags

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

set(KONQUEROR_LIB_VERSION "5.0.97")
set(KONQUEROR_VERSION "${KONQUEROR_LIB_VERSION}")

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Widgets WebEngineWidgets)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Parts KCMUtils KHtml KDELibs4Support Archive Crash)

find_package(KF5 ${KF5_MIN_VERSION} COMPONENTS Activities DocTools) # Optional

find_package(X11)
set(KONQ_HAVE_X11 ${X11_FOUND})

if (X11_FOUND)
  find_package(Qt5 REQUIRED X11Extras)
endif(X11_FOUND)

add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)
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

add_subdirectory( about )
add_subdirectory( pics )
#add_subdirectory( sidebar )
add_subdirectory( settings )
add_subdirectory( plugins )

if (KF5DocTools_FOUND)
    add_subdirectory( doc )
endif()

########### install files ###############
install( PROGRAMS
   kfmclient.desktop kfmclient_html.desktop kfmclient_war.desktop kfmclient_dir.desktop
   konqbrowser.desktop
   DESTINATION  ${KDE_INSTALL_APPDIR} )
install(FILES org.kde.konqueror.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

# Make sure kfmclient can find konqueror.desktop even if people remove it from the K menu (#62242)
# This is why konqueror.desktop is installed into services.
install( FILES org.kde.konqueror.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )

install(FILES konqy_preload.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

install(FILES konqueror.categories DESTINATION ${KDE_INSTALL_CONFDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
