


add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

########### next target ###############

set(rellinksplugin_PART_SRCS plugin_rellinks.cpp )

kde4_add_plugin(rellinksplugin  ${rellinksplugin_PART_SRCS})



target_link_libraries(rellinksplugin  ${KDE4_KHTML_LIBS} )

install(TARGETS rellinksplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_rellinks.rc plugin_rellinks.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )


