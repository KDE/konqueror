project(babelfish)
add_definitions(-DTRANSLATION_DOMAIN=\"babelfish\")
########### babelfishplugin ###############

set(babelfishplugin_PART_SRCS plugin_babelfish.cpp )

add_library(babelfishplugin MODULE ${babelfishplugin_PART_SRCS})

target_link_libraries(babelfishplugin KF5::Parts)

install(TARGETS babelfishplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES plugin_babelfish.rc plugin_translator.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )
install( FILES translaterc  DESTINATION  ${KDE_INSTALL_CONFDIR} )
install( FILES plugin_babelfish.rc plugin_translator.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/kwebkitpart/kpartplugins )
install( FILES plugin_babelfish.rc plugin_translator.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/webenginepart/kpartplugins )

set(babelfish_ICONS 16-actions-babelfish.png 22-actions-babelfish.png)

ecm_install_icons(ICONS ${babelfish_ICONS} DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)
