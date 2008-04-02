########### kfmclient ###############

set(kfmclient_SRCS kfmclient.cpp )

qt4_add_dbus_interface( kfmclient_SRCS ../src/org.kde.Konqueror.Main.xml konq_main_interface )
qt4_add_dbus_interface( kfmclient_SRCS ../src/org.kde.Konqueror.MainWindow.xml konq_mainwindow_interface )


add_definitions(-DQT_NO_CAST_ASCII)

kde4_add_app_icon(kfmclient_SRCS "${KDE4_ICON_INSTALL_DIR}/oxygen/*/apps/system-file-manager.png")

kde4_add_kdeinit_executable( kfmclient NOGUI ${kfmclient_SRCS})

target_link_libraries(kdeinit_kfmclient  ${KDE4_KIO_LIBS} )

install(TARGETS kdeinit_kfmclient  DESTINATION ${INSTALL_TARGETS_DEFAULT_ARGS} )

target_link_libraries( kfmclient kdeinit_kfmclient )
install(TARGETS kfmclient DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############
if( NOT WIN32)
  install( FILES kfmclient_3_2.upd  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
  install( PROGRAMS kfmclient_3_2_update.sh  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
endif( NOT WIN32)
