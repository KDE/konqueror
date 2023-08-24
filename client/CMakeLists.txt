########### kfmclient ###############
include(ECMMarkNonGuiExecutable)

set(kfmclient_SRCS
   kfmclient.cpp
   konqclientrequest.cpp
)

ecm_qt_declare_logging_category(kfmclient_SRCS HEADER kfmclient_debug.h IDENTIFIER KFMCLIENT_LOG CATEGORY_NAME org.kde.kfmclient)

qt_add_dbus_interface( kfmclient_SRCS ../src/org.kde.Konqueror.Main.xml konq_main_interface )
qt_add_dbus_interface( kfmclient_SRCS ../src/org.kde.Konqueror.MainWindow.xml konq_mainwindow_interface )

add_definitions(-DQT_NO_CAST_ASCII)

set(kfmclient_LIBS
   KF${KF_MAJOR_VERSION}::CoreAddons
   KF${KF_MAJOR_VERSION}::KIOCore
   KF${KF_MAJOR_VERSION}::KIOWidgets
   KF${KF_MAJOR_VERSION}::I18n
   KF${KF_MAJOR_VERSION}::WindowSystem
   Qt${KF_MAJOR_VERSION}::DBus
)

if (KF_MAJOR_VERSION STRLESS "6")
    list(APPEND kfmclient_LIBS Qt5::X11Extras)
endif()

if (WIN32)
    add_definitions(-Dkdemain=main)
    add_executable(kfmclient ${kfmclient_SRCS})
    target_link_libraries(kfmclient ${kfmclient_LIBS})
else ()
    if (KF_MAJOR_VERSION STRLESS "6")
        list(APPEND kfmclient_LIBS Qt${KF_MAJOR_VERSION}::X11Extras)
        kf5_add_kdeinit_executable( kfmclient NOGUI ${kfmclient_SRCS})
        target_link_libraries(kdeinit_kfmclient ${kfmclient_LIBS} )
        install(TARGETS kdeinit_kfmclient  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
    else()
        list(APPEND kfmclient_LIBS Qt${KF_MAJOR_VERSION}::GuiPrivate)
        add_executable(kfmclient ${kfmclient_SRCS})
        target_link_libraries(kfmclient ${kfmclient_LIBS})
    endif()
endif ()

install(TARGETS kfmclient ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

