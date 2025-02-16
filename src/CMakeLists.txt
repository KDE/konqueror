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
   KF${KF_MAJOR_VERSION}::KonqSettings
   KF${KF_MAJOR_VERSION}::Parts
   KF${KF_MAJOR_VERSION}::KIOGui
)

set_target_properties(konquerorprivate PROPERTIES VERSION ${KONQUEROR_LIB_VERSION} SOVERSION "${KF_MAJOR_VERSION}" )

target_link_libraries(konquerorprivate Qt${KF_MAJOR_VERSION}::GuiPrivate)

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
   konqembedsettings.cpp
   pluginmetadatautils.cpp
   implementations/konqbrowserwindowinterface.cpp
   konqutils.cpp
   implementations/konqbrowser.cpp
   configdialog.cpp
   placeholderpart.cpp
   fullscreenmanager.cpp
   downloadactionquestion.cpp
   implementations/konqwindow.cpp
)

if (${KActivities_FOUND})
   list(APPEND konqueror_KDEINIT_SRCS activitymanager.cpp)
endif()

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
#We need a library because some tests link to it
add_library(konqueror_internal_lib STATIC ${konqueror_KDEINIT_SRCS})
add_executable(konqueror konqmain.cpp)
target_link_libraries(konqueror konqueror_internal_lib)
set(konqueror_internal_lib_target konqueror_internal_lib)
target_link_libraries(${konqueror_internal_lib_target}
   konquerorprivate
   KF${KF_MAJOR_VERSION}::Archive
   KF${KF_MAJOR_VERSION}::KCMUtils
   KF${KF_MAJOR_VERSION}::Konq
   KF${KF_MAJOR_VERSION}::KonqSettings
   KF${KF_MAJOR_VERSION}::Parts
   KF${KF_MAJOR_VERSION}::DBusAddons
   KF${KF_MAJOR_VERSION}::Crash
   KF${KF_MAJOR_VERSION}::WindowSystem
   KF${KF_MAJOR_VERSION}::SonnetUi
   KF${KF_MAJOR_VERSION}::Bookmarks
   KF${KF_MAJOR_VERSION}::BookmarksWidgets
)

if (KActivities_FOUND)
   target_link_libraries(${konqueror_internal_lib_target} Plasma::Activities)
  target_compile_definitions(${konqueror_internal_lib_target} PUBLIC KActivities_FOUND)
endif ()

install(TARGETS konqueror ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############
# install( FILES konqueror.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
install( FILES org.kde.Konqueror.Main.xml org.kde.Konqueror.MainWindow.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
install(FILES konqautofiltersrc DESTINATION ${KDE_INSTALL_CONFDIR})
