project(KDEBASE_APPS)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

#search packages used by KDE
find_package(KDE4 REQUIRED)
find_package(Strigi REQUIRED)
find_package(ZLIB REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)

include(ConfigureChecks.cmake)
configure_file (config-apps.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-apps.h )
include_directories (${CMAKE_CURRENT_BINARY_DIR})

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/lib/konq ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

if (EXISTS "${kdebase_SOURCE_DIR}")
  # compiling kdebase as a whole, so Plasma is always there
  set (PLASMA_FOUND TRUE)
  set (PLASMA_INCLUDE_DIR ${kdebase_SOURCE_DIR}/workspace/libs)
  set (PLASMA_LIBS plasma)
  set (PLASMA_OPENGL_FOUND ${PLASMA_INCLUDE_DIR}/plasma/glapplet.h)
else (EXISTS "${kdebase_SOURCE_DIR}")
  # compiling kdebase-apps separately, look for already an installed Plasma
  find_package(Plasma)
endif (EXISTS "${kdebase_SOURCE_DIR}")

add_subdirectory( lib )
add_subdirectory( dolphin )
add_subdirectory( kwrite )
add_subdirectory( kdialog )
add_subdirectory( keditbookmarks )
add_subdirectory( konqueror )
add_subdirectory( kfind )
add_subdirectory( kappfinder )

if ( Q_WS_MAC )
  add_subdirectory( konsole )
  add_subdirectory( kdepasswd )
endif ( Q_WS_MAC )

if ( Q_WS_X11 )
  add_subdirectory( kinfocenter )
  add_subdirectory( nsplugins )
  add_subdirectory( konsole )
  add_subdirectory( kdepasswd )
  if ( PLASMA_FOUND )
    add_subdirectory( plasma )
  endif ( PLASMA_FOUND )
endif ( Q_WS_X11 )
macro_optional_add_subdirectory( doc )

if(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    macro_display_feature_log()
endif(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
