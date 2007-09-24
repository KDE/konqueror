add_subdirectory( tests )

check_symbol_exists(mallinfo        "malloc.h"                 KDE_MALLINFO_MALLOC)
check_symbol_exists(mallinfo        "stdlib.h"                 KDE_MALLINFO_STDLIB)
# TODO KDE_MALLINFO_FIELD_hblkhd
# TODO KDE_MALLINFO_FIELD_uordblks
# TODO KDE_MALLINFO_FIELD_usmblks
configure_file(config-konqueror.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konqueror.h )

include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_SOURCE_DIR}/workspace/kcontrol/input )

if (WIN32)
    add_definitions(-DMAKE_KDEINIT_KONQUEROR_LIB)
endif(WIN32)

########### next target ###############

set(konqueror_KDEINIT_SRCS
   konqapplication.cpp
   konqguiclients.cpp
   konqrun.cpp
   konqview.cpp
   konqviewmanager.cpp
   konqmisc.cpp
   konqframe.cpp
   konqframevisitor.cpp
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
   konqbookmarkbar.cpp
   konqsettings.cpp
   KonquerorAdaptor.cpp
   KonqMainWindowAdaptor.cpp
   KonqViewAdaptor.cpp
)
kde4_add_kcfg_files(konqueror_KDEINIT_SRCS konqsettingsxt.kcfgc)

qt4_add_dbus_interface(konqueror_KDEINIT_SRCS org.kde.Konqueror.Main.xml konqueror_interface)


kde4_add_kdeinit_executable(konqueror ${konqueror_KDEINIT_SRCS} konqmain.cpp)

target_link_libraries(kdeinit_konqueror ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KUTILS_LIBS} konq)
target_link_libraries(konqueror kdeinit_konqueror)

install(TARGETS kdeinit_konqueror  DESTINATION ${LIB_INSTALL_DIR} )
install(TARGETS konqueror DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############
install( FILES konqueror.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES konqueror.rc konq-webbrowsing.rc konq-filemanagement.rc DESTINATION  ${DATA_INSTALL_DIR}/konqueror )
install(FILES org.kde.Konqueror.Main.xml org.kde.Konqueror.MainWindow.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
