########### kfmclient ###############
include(ECMMarkNonGuiExecutable)
find_package(KF5 REQUIRED Init)

set(kfmclient_SRCS
   kfmclient.cpp
   konqclientrequest.cpp
)

ecm_qt_declare_logging_category(kfmclient_SRCS HEADER kfmclient_debug.h IDENTIFIER KFMCLIENT_LOG CATEGORY_NAME org.kde.kfmclient)

qt5_add_dbus_interface( kfmclient_SRCS ../src/org.kde.Konqueror.Main.xml konq_main_interface )
qt5_add_dbus_interface( kfmclient_SRCS ../src/org.kde.Konqueror.MainWindow.xml konq_mainwindow_interface )

add_definitions(-DQT_NO_CAST_ASCII)

set(kfmclient_LIBS
   KF5::CoreAddons
   KF5::KIOCore
   KF5::KIOWidgets
   KF5::I18n
   KF5::WindowSystem
   Qt5::DBus
)

if (WIN32)
    add_definitions(-Dkdemain=main)
    add_executable(kfmclient ${kfmclient_SRCS})
    target_link_libraries(kfmclient ${kfmclient_LIBS})
else (WIN32)
    kf5_add_kdeinit_executable( kfmclient NOGUI ${kfmclient_SRCS})
    target_link_libraries(kdeinit_kfmclient ${kfmclient_LIBS} Qt5::X11Extras)
    install(TARGETS kdeinit_kfmclient  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
endif (WIN32)

install(TARGETS kfmclient ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

