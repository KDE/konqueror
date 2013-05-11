project(KDEBASE_APPS)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

include(CTest)
include(CTestConfig.cmake)

#search packages used by KDE
set(KDE_MIN_VERSION "4.10.2")
set(SOPRANO_MIN_VERSION "2.7.56")
find_package(KDE4 4.8.0 REQUIRED)
find_package(Strigi REQUIRED)
find_package(ZLIB REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)

find_package(KActivities 6.1.0)
macro_log_feature(KActivities_FOUND
  "KActivities" "Interface library for the activity manager"
  "http://projects.kde.org/kde/kdelibs/kactivities/" FALSE "6.1.0"
  "STRONGLY_RECOMMENDED: Needed for Dolphin integration with activities"
  )

include(ConfigureChecks.cmake)
configure_file (config-apps.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-apps.h )
include_directories (${CMAKE_CURRENT_BINARY_DIR})

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -DDISABLE_NEPOMUK_LEGACY)
add_definitions (-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/lib/konq ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

if ( KActivities_FOUND )
  include_directories (${KACTIVITIES_INCLUDE_DIR})
endif ( KActivities_FOUND )

add_subdirectory( lib )
add_subdirectory( dolphin )
add_subdirectory( kdialog )
add_subdirectory( keditbookmarks )
add_subdirectory( konqueror )
add_subdirectory( kfind )
add_subdirectory( plasma )
add_subdirectory( konq-plugins )

if ( Q_WS_MAC )
  add_subdirectory( kdepasswd )
endif ( Q_WS_MAC )

if ( Q_WS_X11 )
  add_subdirectory( nsplugins )
  add_subdirectory( kdepasswd )
endif ( Q_WS_X11 )
macro_optional_add_subdirectory( doc )

if(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    macro_display_feature_log()
endif(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
