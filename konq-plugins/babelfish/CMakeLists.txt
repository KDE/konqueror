project(babelfish)

########### babelfishplugin ###############

set(babelfishplugin_PART_SRCS plugin_babelfish.cpp )

kde4_add_plugin(babelfishplugin ${babelfishplugin_PART_SRCS})

target_link_libraries(babelfishplugin  ${KDE4_KPARTS_LIBS} )

install(TARGETS babelfishplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_babelfish.rc plugin_translator.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES translaterc  DESTINATION  ${CONFIG_INSTALL_DIR} )
install( FILES plugin_babelfish.rc plugin_translator.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart/kpartplugins )

kde4_install_icons(${ICON_INSTALL_DIR})
