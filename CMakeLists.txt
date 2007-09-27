project(KDEBASE_APPS)

#search packages used by KDE
find_package(KDE4 REQUIRED)
find_package(Strigi REQUIRED)
find_package(Blitz REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)
include(MacroOptionalAddSubdirectory)

include(ConfigureChecks.cmake)
configure_file (config-apps.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-apps.h )
include_directories (${CMAKE_CURRENT_BINARY_DIR})

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/lib/konq ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

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
  add_subdirectory( nsplugins )
  add_subdirectory( konsole )
  add_subdirectory( kdepasswd )
endif ( Q_WS_X11 )

add_subdirectory( doc )

