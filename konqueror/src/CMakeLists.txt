if(KDE4_BUILD_TESTS)
    # only with this definition will the KONQ_TESTS_EXPORT macro do something
    add_definitions(-DCOMPILING_TESTS)
endif(KDE4_BUILD_TESTS)

add_subdirectory( tests )

check_symbol_exists(mallinfo        "malloc.h"                 KDE_MALLINFO_MALLOC)
check_symbol_exists(mallinfo        "stdlib.h"                 KDE_MALLINFO_STDLIB)
# TODO KDE_MALLINFO_FIELD_hblkhd
# TODO KDE_MALLINFO_FIELD_uordblks
# TODO KDE_MALLINFO_FIELD_usmblks
configure_file(config-konqueror.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konqueror.h )

if (WIN32)
    add_definitions(-DMAKE_KDEINIT_KONQUEROR_LIB)
endif(WIN32)

########### libkonquerorprivate, shared with sidebar modules ###############

set(konquerorprivate_SRCS
   konqpixmapprovider.cpp
)
qt4_add_dbus_interface(konquerorprivate_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/konq/favicons/org.kde.FavIcon.xml favicon_interface)
kde4_add_library(konquerorprivate SHARED ${konquerorprivate_SRCS})
target_link_libraries(konquerorprivate ${KDE4_KDEUI_LIBS})

########### konqueror ###############

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
   konqproxystyle.cpp
   konqundomanager.cpp
)
kde4_add_kcfg_files(konqueror_KDEINIT_SRCS konqsettingsxt.kcfgc)

qt4_add_dbus_interface(konqueror_KDEINIT_SRCS org.kde.Konqueror.Main.xml konqueror_interface)
qt4_add_dbus_interface(konqueror_KDEINIT_SRCS ${DBUS_INTERFACES_INSTALL_DIR}/org.kde.kded.xml kded_interface)
kde4_add_kdeinit_executable(konqueror ${konqueror_KDEINIT_SRCS} konqmain.cpp)

# KDE4_KUTILS_LIBS used for KCMultiDialog, KCModuleInfo, and KPluginSelector
target_link_libraries(kdeinit_konqueror konquerorprivate konq ${KDE4_KPARTS_LIBS} ${KDE4_KUTILS_LIBS})
target_link_libraries(konqueror kdeinit_konqueror)

install(TARGETS kdeinit_konqueror  DESTINATION ${LIB_INSTALL_DIR} )
install(TARGETS konqueror DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############
install( FILES konqueror.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES konqueror.rc konq-webbrowsing.rc konq-filemanagement.rc DESTINATION  ${DATA_INSTALL_DIR}/konqueror )
install(FILES org.kde.Konqueror.Main.xml org.kde.Konqueror.MainWindow.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
