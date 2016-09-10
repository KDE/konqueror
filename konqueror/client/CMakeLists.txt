########### kfmclient ###############
include(ECMMarkNonGuiExecutable)
find_package(KF5 REQUIRED Init)

set(kfmclient_SRCS kfmclient.cpp )

qt5_add_dbus_interface( kfmclient_SRCS ../src/org.kde.Konqueror.Main.xml konq_main_interface )
qt5_add_dbus_interface( kfmclient_SRCS ../src/org.kde.Konqueror.MainWindow.xml konq_mainwindow_interface )


add_definitions(-DQT_NO_CAST_ASCII)

kde4_add_app_icon(kfmclient_SRCS "${KDE4_ICON_INSTALL_DIR}/oxygen/*/apps/system-file-manager.png")

if (WIN32)
    add_definitions(-Dkdemain=main)
    add_executable(kfmclient ${kfmclient_SRCS})
    target_link_libraries(kfmclient  KF5::KIOCore )
else (WIN32)
    kf5_add_kdeinit_executable( kfmclient NOGUI ${kfmclient_SRCS})
    target_link_libraries(kdeinit_kfmclient KF5::KIOCore KF5::KDELibs4Support)
    if(X11_FOUND)
      target_link_libraries(kdeinit_kfmclient Qt5::X11Extras ${X11_LIBRARIES})
    endif(X11_FOUND)
    install(TARGETS kdeinit_kfmclient  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
endif (WIN32)

install(TARGETS kfmclient ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

