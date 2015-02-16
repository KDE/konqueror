


########### next target ###############

find_package(KF5 REQUIRED KHtml)

set(rellinksplugin_PART_SRCS plugin_rellinks.cpp )

add_library(rellinksplugin MODULE ${rellinksplugin_PART_SRCS})



target_link_libraries(rellinksplugin  KF5::KHtml KF5::KDELibs4Support)

install(TARGETS rellinksplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_rellinks.rc plugin_rellinks.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )


