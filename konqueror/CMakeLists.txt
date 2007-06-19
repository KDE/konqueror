
project(konqueror)

add_definitions(-D_LARGEFILE64_SOURCE )

add_definitions (-DQT3_SUPPORT)

check_symbol_exists(mallinfo        "malloc.h"                 KDE_MALLINFO_MALLOC)
check_symbol_exists(mallinfo        "stdlib.h"                 KDE_MALLINFO_STDLIB)
# TODO KDE_MALLINFO_FIELD_hblkhd
# TODO KDE_MALLINFO_FIELD_uordblks
# TODO KDE_MALLINFO_FIELD_usmblks
configure_file(config-konqueror.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konqueror.h )

add_subdirectory( client )
# on the way out
#add_subdirectory( iconview )
#add_subdirectory( listview )

if(UNIX)
   add_subdirectory( shellcmdplugin )
endif(UNIX)

add_subdirectory( about )
add_subdirectory( pics )
add_subdirectory( sidebar )
add_subdirectory( preloader )
add_subdirectory( quickprint )
add_subdirectory( remoteencodingplugin )
add_subdirectory( dirpart )
add_subdirectory( settings )

add_subdirectory( kttsplugin )

include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/workspace/kcontrol/input )
add_subdirectory( tests )


########### next target ###############

set(konquerorprivate_SRCS
   konq_application.cpp
   konq_guiclients.cc
   konq_run.cc
   konq_view.cc
   konq_viewmgr.cc
   konq_misc.cc
   konq_frame.cc
   konq_framecontainer.cc
   konq_tabs.cc
   konq_actions.cc
   konq_profiledlg.cc
   konq_factory.cc
   konq_combo.cc
   konq_browseriface.cc
   delayedinitializer.cc
   konq_mainwindow.cc
   konq_extensionmanager.cc
   KonquerorAdaptor.cpp
   KonqMainWindowAdaptor.cpp
   KonqViewAdaptor.cpp
)
kde4_add_kcfg_files(konquerorprivate_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/konq_settingsxt.kcfgc)

kde4_automoc(${konquerorprivate_SRCS})

qt4_add_dbus_interface(konquerorprivate_SRCS org.kde.Konqueror.Main.xml konqueror_interface)

kde4_add_library(konquerorprivate ${konquerorprivate_SRCS})
target_link_libraries(konquerorprivate ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KUTILS_LIBS} konq)

kde4_add_kdeinit_executable( konqueror konq_main.cc)
target_link_libraries(kdeinit_konqueror konquerorprivate )
target_link_libraries( konqueror kdeinit_konqueror )
install(TARGETS kdeinit_konqueror  DESTINATION ${LIB_INSTALL_DIR} )
install(TARGETS konqueror DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############
install( FILES konqfilemgr.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kfmclient.desktop kfmclient_dir.desktop kfmclient_html.desktop  	kfmclient_war.desktop konqbrowser.desktop konquerorsu.desktop Home.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES konqueror.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES konqueror.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES konqueror.rc konq-simplebrowser.rc  DESTINATION  ${DATA_INSTALL_DIR}/konqueror )

install(FILES profile_webbrowsing.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME webbrowsing)
install(FILES profile_filemanagement.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME filemanagement)
if(NOT WIN32)
install(FILES profile_midnightcommander.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME midnightcommander)
endif(NOT WIN32)
install(FILES profile_tabbedbrowsing.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME tabbedbrowsing)
install(FILES profile_kde_devel.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME kde_devel)
install(FILES profile_simplebrowser.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME simplebrowser)
install(FILES org.kde.Konqueror.Main.xml org.kde.Konqueror.MainWindow.xml DESTINATION ${DBUS_INTERFACES_DIR} )
