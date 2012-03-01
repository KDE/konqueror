



########### next target ###############

set(khtmlsettingsplugin_PART_SRCS settingsplugin.cpp )

kde4_add_plugin(khtmlsettingsplugin  ${khtmlsettingsplugin_PART_SRCS})



target_link_libraries(khtmlsettingsplugin  ${KDE4_KPARTS_LIBS} )

install(TARGETS khtmlsettingsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES khtmlsettingsplugin.rc khtmlsettingsplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES khtmlsettingsplugin.rc khtmlsettingsplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart/kpartplugins )

