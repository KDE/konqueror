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

# For crc32 in konqhistorymanager.cpp
include_directories( ${ZLIB_INCLUDE_DIR} )

########### libkonquerorprivate, shared with sidebar modules ###############

set(konquerorprivate_SRCS
   konqpixmapprovider.cpp
   konqhistorymanager.cpp
   konqhistoryentry.cpp
)
qt4_add_dbus_interface(konquerorprivate_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/konq/favicons/org.kde.FavIcon.xml favicon_interface)
qt4_add_dbus_adaptor(konquerorprivate_SRCS org.kde.Konqueror.HistoryManager.xml konqhistorymanager.h KonqHistoryManager konqhistorymanageradaptor KonqHistoryManagerAdaptor)

kde4_add_library(konquerorprivate SHARED ${konquerorprivate_SRCS})
target_link_libraries(konquerorprivate ${KDE4_KPARTS_LIBS} ${ZLIB_LIBRARY})
set_target_properties(konquerorprivate PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS konquerorprivate ${INSTALL_TARGETS_DEFAULT_ARGS})

########### konqueror ###############
if (WIN32)
    add_definitions(-DMAKE_KDEINIT_KONQUEROR_LIB)
endif(WIN32)

set(konqueror_KDEINIT_SRCS
   konqapplication.cpp
   konqguiclients.cpp
   konqrun.cpp
   konqview.cpp
   konqviewmanager.cpp
   konqmisc.cpp
   konqdraggablelabel.cpp
   konqframe.cpp
   konqframevisitor.cpp
   konqframestatusbar.cpp
   konqframecontainer.cpp
   konqtabs.cpp
   konqactions.cpp
   konqprofiledlg.cpp
   konqsessiondlg.cpp
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
   konqsessionmanager.cpp
   konqcloseditem.cpp
)

kde4_add_kcfg_files(konqueror_KDEINIT_SRCS konqsettingsxt.kcfgc)
kde4_add_ui_files(konqueror_KDEINIT_SRCS konqprofiledlg_base.ui)
kde4_add_ui_files(konqueror_KDEINIT_SRCS konqsessiondlg_base.ui)
kde4_add_ui_files(konqueror_KDEINIT_SRCS konqnewsessiondlg_base.ui)

qt4_add_dbus_interface(konqueror_KDEINIT_SRCS org.kde.Konqueror.Main.xml konqueror_interface)
qt4_add_dbus_interface(konqueror_KDEINIT_SRCS ${KDE4_DBUS_INTERFACES_DIR}/org.kde.kded.xml kded_interface)
qt4_add_dbus_adaptor(konqueror_KDEINIT_SRCS org.kde.Konqueror.UndoManager.xml konqundomanager.h KonqClosedWindowsManager konqundomanageradaptor KonqClosedWindowsManagerAdaptor)
qt4_add_dbus_interface(konqueror_KDEINIT_SRCS org.kde.Konqueror.UndoManager.xml konqundomanager_interface)
qt4_add_dbus_adaptor(konqueror_KDEINIT_SRCS org.kde.Konqueror.SessionManager.xml konqsessionmanager.h KonqSessionManager konqsessionmanageradaptor KonqSessionManagerAdaptor)
qt4_add_dbus_interface(konqueror_KDEINIT_SRCS org.kde.Konqueror.SessionManager.xml konqsessionmanager_interface)

kde4_add_app_icon(konqueror_KDEINIT_SRCS "${KDE4_ICON_INSTALL_DIR}/oxygen/*/apps/konqueror.png")
kde4_add_kdeinit_executable(konqueror ${konqueror_KDEINIT_SRCS} konqmain.cpp)
target_link_libraries(kdeinit_konqueror konquerorprivate konq ${KDE4_KPARTS_LIBS} ${KDE4_KUTILS_LIBS})
if (UNIX)
  target_link_libraries(kdeinit_konqueror ${X11_LIBRARIES})
endif (UNIX)


if (NOT WIN32)
    install(TARGETS kdeinit_konqueror  ${INSTALL_TARGETS_DEFAULT_ARGS} )
endif (NOT WIN32)
install(TARGETS konqueror ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############
install( FILES konqueror.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES konqueror.rc konq-webbrowsing.rc konq-filemanagement.rc DESTINATION  ${DATA_INSTALL_DIR}/konqueror )
install( FILES org.kde.Konqueror.Main.xml org.kde.Konqueror.MainWindow.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
