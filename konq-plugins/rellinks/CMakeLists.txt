


add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

########### next target ###############

set(rellinksplugin_PART_SRCS plugin_rellinks.cpp )

add_library(rellinksplugin MODULE ${rellinksplugin_PART_SRCS})



target_link_libraries(rellinksplugin  KF5::KHtml )

install(TARGETS rellinksplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_rellinks.rc plugin_rellinks.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )


