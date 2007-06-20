add_subdirectory( tests )

check_symbol_exists(mallinfo        "malloc.h"                 KDE_MALLINFO_MALLOC)
check_symbol_exists(mallinfo        "stdlib.h"                 KDE_MALLINFO_STDLIB)
# TODO KDE_MALLINFO_FIELD_hblkhd
# TODO KDE_MALLINFO_FIELD_uordblks
# TODO KDE_MALLINFO_FIELD_usmblks
configure_file(config-konqueror.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konqueror.h )

include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/workspace/kcontrol/input )

########### next target ###############

set(konquerorprivate_SRCS
   konqapplication.cpp
   konqguiclients.cpp
   konqrun.cpp
   konqview.cpp
   konqviewmanager.cpp
   konqmisc.cpp
   konqframe.cpp
   konqframestatusbar.cpp
   konqframecontainer.cpp
   konqtabs.cpp
   konqactions.cpp
   konqprofiledlg.cpp
   konqfactory.cpp
   konqcombo.cpp
   konqbrowseriface.cpp
   delayedinitializer.cpp
   konqmainwindow.cpp
   konqextensionmanager.cpp
   KonquerorAdaptor.cpp
   KonqMainWindowAdaptor.cpp
   KonqViewAdaptor.cpp
)
kde4_add_kcfg_files(konquerorprivate_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/konqsettingsxt.kcfgc)

kde4_automoc(${konquerorprivate_SRCS})

qt4_add_dbus_interface(konquerorprivate_SRCS org.kde.Konqueror.Main.xml konqueror_interface)

kde4_add_library(konquerorprivate SHARED ${konquerorprivate_SRCS})
# This lib is private. Don't even think about linking to it!!!
# It *only* exists so that unit tests can be written.
target_link_libraries(konquerorprivate ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KUTILS_LIBS} konq)
set_target_properties(konquerorprivate PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS konquerorprivate DESTINATION ${LIB_INSTALL_DIR})


kde4_add_kdeinit_executable( konqueror konqmain.cpp)
target_link_libraries(kdeinit_konqueror konquerorprivate )
target_link_libraries( konqueror kdeinit_konqueror )
install(TARGETS kdeinit_konqueror  DESTINATION ${LIB_INSTALL_DIR} )
install(TARGETS konqueror DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############
install( FILES konqueror.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES konqueror.rc konq-simplebrowser.rc  DESTINATION  ${DATA_INSTALL_DIR}/konqueror )
install(FILES org.kde.Konqueror.Main.xml org.kde.Konqueror.MainWindow.xml DESTINATION ${DBUS_INTERFACES_DIR} )
