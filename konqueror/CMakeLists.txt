cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(Konqueror)

set(QT_MIN_VERSION "5.4.0")
set(KF5_MIN_VERSION "5.27.0")

find_package(ECM 1.7.0 CONFIG REQUIRED)

include(FeatureSummary)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMSetupVersion)

set(KONQUEROR_LIB_VERSION "5.0.97")

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Widgets WebEngineWidgets)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Parts KCMUtils KHtml KDELibs4Support Archive Crash)

find_package(KF5 ${KF5_MIN_VERSION} COMPONENTS Activities) # Optional

find_package(KDED REQUIRED)

find_package(X11)
set(KONQ_HAVE_X11 ${X11_FOUND})

if (X11_FOUND)
  find_package(Qt5 REQUIRED X11Extras)
endif(X11_FOUND)

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

configure_file (config-konqueror.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konqueror.h )

add_subdirectory( libkonq )
add_subdirectory( src )
add_subdirectory( client )
add_subdirectory( autotests )
add_subdirectory( kwebenginepart )

add_subdirectory( about )
add_subdirectory( pics )
#add_subdirectory( sidebar )
add_subdirectory( settings )
add_subdirectory( plugins )

########### install files ###############
install( PROGRAMS
   kfmclient.desktop kfmclient_html.desktop kfmclient_war.desktop
   konqbrowser.desktop
   DESTINATION  ${KDE_INSTALL_APPDIR} )
install(FILES konqueror.appdata.xml DESTINATION ${KDE_INSTALL_DATAROOTDIR}/appdata )

# Make sure kfmclient can find konqueror.desktop even if people remove it from the K menu (#62242)
# This is why konqueror.desktop is installed into services.
install( FILES konqueror.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )

install(FILES konqy_preload.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})

# TODO Remove the if/endif lines if Konqueror is split.
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${Konqueror_SOURCE_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
