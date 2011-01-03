



########### next target ###############

set(minitoolsplugin_PART_SRCS minitoolsplugin.cpp )

kde4_add_plugin(minitoolsplugin ${minitoolsplugin_PART_SRCS})



target_link_libraries(minitoolsplugin  ${KDE4_KHTML_LIBS} )

install(TARGETS minitoolsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES minitoolsplugin.rc minitoolsplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )

