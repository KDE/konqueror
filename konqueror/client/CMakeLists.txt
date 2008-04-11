########### kfmclient ###############

set(kfmclient_SRCS kfmclient.cpp )

qt4_add_dbus_interface( kfmclient_SRCS ../src/org.kde.Konqueror.Main.xml konq_main_interface )
qt4_add_dbus_interface( kfmclient_SRCS ../src/org.kde.Konqueror.MainWindow.xml konq_mainwindow_interface )


add_definitions(-DQT_NO_CAST_ASCII)

kde4_add_app_icon(kfmclient_SRCS "${KDE4_ICON_INSTALL_DIR}/oxygen/*/apps/system-file-manager.png")

if (WIN32)
    add_definitions(-Dkdemain=main)
    kde4_add_executable(kfmclient ${kfmclient_SRCS})
    target_link_libraries(kfmclient  ${KDE4_KIO_LIBS} )
else (WIN32)
    kde4_add_kdeinit_executable( kfmclient NOGUI ${kfmclient_SRCS})
    target_link_libraries(kdeinit_kfmclient  ${KDE4_KIO_LIBS} )
    install(TARGETS kdeinit_kfmclient  ${INSTALL_TARGETS_DEFAULT_ARGS} )
endif (WIN32)

install(TARGETS kfmclient ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############
if( NOT WIN32)
  install( FILES kfmclient_3_2.upd  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
  install( PROGRAMS kfmclient_3_2_update.sh  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
endif( NOT WIN32)
