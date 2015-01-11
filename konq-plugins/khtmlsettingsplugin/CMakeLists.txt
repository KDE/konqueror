



########### next target ###############

set(khtmlsettingsplugin_PART_SRCS settingsplugin.cpp )

add_library(khtmlsettingsplugin MODULE ${khtmlsettingsplugin_PART_SRCS})



target_link_libraries(khtmlsettingsplugin  KF5::Parts )

install(TARGETS khtmlsettingsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES khtmlsettingsplugin.rc khtmlsettingsplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES khtmlsettingsplugin.rc khtmlsettingsplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart/kpartplugins )

