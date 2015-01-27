project(babelfish)

########### babelfishplugin ###############

set(babelfishplugin_PART_SRCS plugin_babelfish.cpp )

add_library(babelfishplugin MODULE ${babelfishplugin_PART_SRCS})

target_link_libraries(babelfishplugin KF5::Parts KF5::KDELibs4Support)

install(TARGETS babelfishplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_babelfish.rc plugin_translator.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES translaterc  DESTINATION  ${CONFIG_INSTALL_DIR} )
install( FILES plugin_babelfish.rc plugin_translator.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart/kpartplugins )

set(babelfish_ICONS hi16-actions-babelfish.png hi22-actions-babelfish.png)

ecm_install_icons(ICONS ${babelfish_ICONS} DESTINATION ${ICON_INSTALL_DIR} THEME hicolor)
