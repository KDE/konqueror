



########### next target ###############

find_package(KF5 REQUIRED KHtml)

set(minitoolsplugin_PART_SRCS minitoolsplugin.cpp )

add_library(minitoolsplugin MODULE ${minitoolsplugin_PART_SRCS})



target_link_libraries(minitoolsplugin  KF5::KHtml KF5::KDELibs4Support)

install(TARGETS minitoolsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES minitoolsplugin.rc minitoolsplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )

