if(BUILD_TESTING)
    # only with this definition will the KONQ_TESTS_EXPORT macro do something
    add_definitions(-DCOMPILING_TESTS)
endif(BUILD_TESTING)

add_subdirectory( tests )

add_definitions(-DTRANSLATION_DOMAIN=\"konqueror\")

include (CheckSymbolExists)
check_symbol_exists(mallinfo        "malloc.h"                 KDE_MALLINFO_MALLOC)
check_symbol_exists(mallinfo        "stdlib.h"                 KDE_MALLINFO_STDLIB)
# TODO KDE_MALLINFO_FIELD_hblkhd
# TODO KDE_MALLINFO_FIELD_uordblks
# TODO KDE_MALLINFO_FIELD_usmblks
configure_file(config-konqueror.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konqueror.h )

if (KActivities_FOUND)
  include_directories(${KACTIVITIES_INCLUDE_DIRS})
endif (KActivities_FOUND)

########### libkonquerorprivate, shared with unit tests and sidebar modules ###############

set(konquerorprivate_SRCS
   konqhistorymanager.cpp # for unit tests
   konqpixmapprovider.cpp # needed ?!?

   # for the sidebar history module
   konqhistorymodel.cpp
   ksortfilterproxymodel.cpp
   konqhistoryproxymodel.cpp
   konqhistoryview.cpp
   konqhistorysettings.cpp
)

add_library(konquerorprivate SHARED ${konquerorprivate_SRCS})
target_link_libraries(konquerorprivate
   KF5::Konq
   KF5::Parts
   KF5::KIOGui
)

if(X11_FOUND)
  target_link_libraries(konquerorprivate Qt5::X11Extras)
endif(X11_FOUND)


set_target_properties(konquerorprivate PROPERTIES VERSION ${KONQUEROR_LIB_VERSION} SOVERSION "5" )
install(TARGETS konquerorprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

include(GenerateExportHeader)
generate_export_header(konquerorprivate BASE_NAME konquerorprivate EXPORT_FILE_NAME konquerorprivate_export.h)

########### konqueror ###############

set(konqueror_KDEINIT_SRCS
   konqapplication.cpp
   konqguiclients.cpp
   konqrun.cpp
   konqview.cpp
   konqviewmanager.cpp
   konqrmbeventfilter.cpp
   konqmisc.cpp
   konqdraggablelabel.cpp
   konqframe.cpp
   konqframevisitor.cpp
   konqframestatusbar.cpp
   konqframecontainer.cpp
   konqtabs.cpp
   konqactions.cpp
   konqsessiondlg.cpp
   konqfactory.cpp
   konqcombo.cpp
   konqbrowseriface.cpp
   delayedinitializer.cpp
   konqanimatedlogo.cpp
   konqmainwindow.cpp
   konqmainwindowfactory.cpp
   konqextensionmanager.cpp
   konqbookmarkbar.cpp
   konqsettings.cpp
   KonquerorAdaptor.cpp
   KonqMainWindowAdaptor.cpp
   KonqViewAdaptor.cpp
   konqundomanager.cpp
   konqclosedwindowsmanager.cpp
   konqsessionmanager.cpp
   konqcloseditem.cpp
   konqhistorydialog.cpp
   konq_statusbarmessagelabel.cpp
)

kconfig_add_kcfg_files(konqueror_KDEINIT_SRCS konqsettingsxt.kcfgc)
ki18n_wrap_ui(konqueror_KDEINIT_SRCS konqsessiondlg_base.ui)
ki18n_wrap_ui(konqueror_KDEINIT_SRCS konqnewsessiondlg_base.ui)

qt5_add_dbus_interface(konqueror_KDEINIT_SRCS org.kde.Konqueror.Main.xml konqueror_interface)
qt5_add_dbus_interface(konqueror_KDEINIT_SRCS ${KDED_DBUS_INTERFACE} kded_interface)
qt5_add_dbus_adaptor(konqueror_KDEINIT_SRCS org.kde.Konqueror.UndoManager.xml konqclosedwindowsmanager.h KonqClosedWindowsManager konqclosedwindowsmanageradaptor KonqClosedWindowsManagerAdaptor)
qt5_add_dbus_interface(konqueror_KDEINIT_SRCS org.kde.Konqueror.UndoManager.xml konqclosedwindowsmanager_interface)
qt5_add_dbus_adaptor(konqueror_KDEINIT_SRCS org.kde.Konqueror.SessionManager.xml konqsessionmanager.h KonqSessionManager konqsessionmanageradaptor KonqSessionManagerAdaptor)
qt5_add_dbus_interface(konqueror_KDEINIT_SRCS org.kde.Konqueror.SessionManager.xml konqsessionmanager_interface)

kde4_add_app_icon(konqueror_KDEINIT_SRCS "${KDE4_ICON_INSTALL_DIR}/oxygen/*/apps/konqueror.png")
kf5_add_kdeinit_executable(konqueror ${konqueror_KDEINIT_SRCS} konqmain.cpp)
target_link_libraries(kdeinit_konqueror
   konquerorprivate
   KF5::Archive
   KF5::KCMUtils
   KF5::Konq
   KF5::Parts
   KF5::KDELibs4Support
   KF5::Crash
)

# For khtmldefaults.h. We don't need to link to khtml, just to include that header...
get_target_property(khtml_include_dir KF5::KHtml INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(kdeinit_konqueror PRIVATE "${khtml_include_dir}")

if (X11_FOUND)
  target_link_libraries(kdeinit_konqueror ${X11_LIBRARIES})
endif ()

if (KActivities_FOUND)
  target_link_libraries(kdeinit_konqueror ${KACTIVITIES_LIBRARY})
endif (KActivities_FOUND)

if (NOT WIN32)
    install(TARGETS kdeinit_konqueror  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
endif (NOT WIN32)
install(TARGETS konqueror ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############
install( FILES konqueror.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
install( FILES konqueror.rc DESTINATION  ${KDE_INSTALL_KXMLGUI5DIR}/konqueror )
install( FILES org.kde.Konqueror.Main.xml org.kde.Konqueror.MainWindow.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
