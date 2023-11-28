if(BUILD_TESTING)
    # only with this definition will the KONQ_TESTS_EXPORT macro do something
    add_definitions(-DBUILD_TESTING)

    add_subdirectory( tests )
endif()

add_definitions(-DTRANSLATION_DOMAIN=\"konqueror\")

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
   konqurl.cpp
)

ecm_qt_declare_logging_category(konquerorprivate_SRCS HEADER konqdebug.h IDENTIFIER KONQUEROR_LOG CATEGORY_NAME org.kde.konqueror)

add_library(konquerorprivate SHARED ${konquerorprivate_SRCS})
target_link_libraries(konquerorprivate
   KF${KF_MAJOR_VERSION}::I18n
   KF${KF_MAJOR_VERSION}::IconThemes
   KF${KF_MAJOR_VERSION}::Konq
   KF${KF_MAJOR_VERSION}::Parts
   KF${KF_MAJOR_VERSION}::KIOGui
)

set_target_properties(konquerorprivate PROPERTIES VERSION ${KONQUEROR_LIB_VERSION} SOVERSION "${KF_MAJOR_VERSION}" )

if (QT_MAJOR_VERSION STRLESS "6")
   target_link_libraries(konquerorprivate Qt5::X11Extras)
else()
   target_link_libraries(konquerorprivate Qt${KF_MAJOR_VERSION}::GuiPrivate)
endif()

set_target_properties(konquerorprivate PROPERTIES VERSION ${KONQUEROR_LIB_VERSION} SOVERSION "5" )

install(TARGETS konquerorprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

generate_export_header(konquerorprivate BASE_NAME konquerorprivate EXPORT_FILE_NAME konquerorprivate_export.h)

########### konqueror ###############

#TODO KF6: rename konqueror_KDEINIT_SRCS as konqueror_SRCS

set(konqueror_KDEINIT_SRCS
   konqapplication.cpp
   konqdebug.cpp
   konqguiclients.cpp
   konqbookmarkmenu.cpp
   konqview.cpp
   konqviewmanager.cpp
   konqmouseeventfilter.cpp
   konqmisc.cpp
   konqdraggablelabel.cpp
   konqframe.cpp
   konqframevisitor.cpp
   konqframestatusbar.cpp
   konqframecontainer.cpp
   ktabbar.cpp
   ktabwidget.cpp
   konqtabs.cpp
   konqactions.cpp
   konqsessiondlg.cpp
   konqfactory.cpp
   konqcombo.cpp
   delayedinitializer.cpp
   konqanimatedlogo.cpp
   konqmainwindow.cpp
   konqmainwindowfactory.cpp
   konqextensionmanager.cpp
   konqbookmarkbar.cpp
   KonquerorAdaptor.cpp
   KonqMainWindowAdaptor.cpp
   KonqViewAdaptor.cpp
   konqundomanager.cpp
   konqclosedwindowsmanager.cpp
   konqsessionmanager.cpp
   konqcloseditem.cpp
   konqhistorydialog.cpp
   konqstatusbarmessagelabel.cpp
   konqurl.cpp
   urlloader.cpp
   konqsettings.cpp
   pluginmetadatautils.cpp
   implementations/konqbrowserwindowinterface.cpp
   konqutils.cpp
   implementations/konqbrowser.cpp
   configdialog.cpp
   placeholderpart.cpp
)

if (${KActivities_FOUND})
   list(APPEND konqueror_KDEINIT_SRCS activitymanager.cpp)
endif()

kconfig_add_kcfg_files(konqueror_KDEINIT_SRCS konqsettingsxt.kcfgc)
ki18n_wrap_ui(konqueror_KDEINIT_SRCS konqsessiondlg_base.ui)
ki18n_wrap_ui(konqueror_KDEINIT_SRCS konqnewsessiondlg_base.ui)
qt_add_resources(konqueror_KDEINIT_SRCS konqueror.qrc)

qt_add_dbus_interface(konqueror_KDEINIT_SRCS org.kde.Konqueror.Main.xml konqueror_interface)
qt_add_dbus_adaptor(konqueror_KDEINIT_SRCS org.kde.Konqueror.UndoManager.xml konqclosedwindowsmanager.h KonqClosedWindowsManager konqclosedwindowsmanageradaptor KonqClosedWindowsManagerAdaptor)
qt_add_dbus_interface(konqueror_KDEINIT_SRCS org.kde.Konqueror.UndoManager.xml konqclosedwindowsmanager_interface)
qt_add_dbus_adaptor(konqueror_KDEINIT_SRCS org.kde.Konqueror.SessionManager.xml konqsessionmanager.h KonqSessionManager konqsessionmanageradaptor KonqSessionManagerAdaptor)
qt_add_dbus_interface(konqueror_KDEINIT_SRCS org.kde.Konqueror.SessionManager.xml konqsessionmanager_interface)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../pics/*-apps-konqueror.png")
ecm_add_app_icon(konqueror_KDEINIT_SRCS ICONS ${ICONS_SRCS})

#TODO KF6: when removing compatibility with KF5, remove konqueror_internal_lib_target and use konqueror_internal_lib
if (KF_MAJOR_VERSION STRLESS "6")
   kf5_add_kdeinit_executable(konqueror ${konqueror_KDEINIT_SRCS} konqmain.cpp)
   add_library(konqueror_internal_lib ALIAS kdeinit_konqueror)
   set(konqueror_internal_lib_target kdeinit_konqueror)
else()
#We need a library because some tests link to it
   add_library(konqueror_internal_lib STATIC ${konqueror_KDEINIT_SRCS})
   add_executable(konqueror konqmain.cpp)
   target_link_libraries(konqueror konqueror_internal_lib)
   set(konqueror_internal_lib_target konqueror_internal_lib)
endif()
target_link_libraries(${konqueror_internal_lib_target}
   konquerorprivate
   KF${KF_MAJOR_VERSION}::Archive
   KF${KF_MAJOR_VERSION}::KCMUtils
   KF${KF_MAJOR_VERSION}::Konq
   KF${KF_MAJOR_VERSION}::Parts
   KF${KF_MAJOR_VERSION}::DBusAddons
   KF${KF_MAJOR_VERSION}::Crash
   KF${KF_MAJOR_VERSION}::WindowSystem
   KF${KF_MAJOR_VERSION}::SonnetUi
   KF${KF_MAJOR_VERSION}::Bookmarks
)

if (KF_MAJOR_VERSION STRGREATER "5")
    target_link_libraries(${konqueror_internal_lib_target} KF${KF_MAJOR_VERSION}::BookmarksWidgets)
endif()

if (KActivities_FOUND)
   if (KF_MAJOR_VERSION STRGREATER "5")
      target_link_libraries(${konqueror_internal_lib_target} Plasma::Activities)
   else()
      target_link_libraries(${konqueror_internal_lib_target} KF${KF_MAJOR_VERSION}::Activities)
   endif()
  target_compile_definitions(${konqueror_internal_lib_target} PUBLIC KActivities_FOUND)
endif ()

if (NOT WIN32 AND (QT_MAJOR_VERSION STRLESS 6))
    install(TARGETS kdeinit_konqueror  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
endif ()
install(TARGETS konqueror ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############
install( FILES konqueror.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
install( FILES org.kde.Konqueror.Main.xml org.kde.Konqueror.MainWindow.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
