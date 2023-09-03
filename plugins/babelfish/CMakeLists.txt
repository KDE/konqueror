project(babelfish)
add_definitions(-DTRANSLATION_DOMAIN=\"babelfish\")
########### babelfishplugin ###############

set(babelfishplugin_PART_SRCS plugin_babelfish.cpp )

add_library(babelfishplugin MODULE ${babelfishplugin_PART_SRCS})

target_link_libraries(babelfishplugin KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts KF${KF_MAJOR_VERSION}Konq)

konqueror_install_plugin_in_namespaces(babelfishplugin)
install(FILES translaterc DESTINATION ${KDE_INSTALL_CONFDIR})

set(babelfish_ICONS 16-actions-babelfish.png 22-actions-babelfish.png)

ecm_install_icons(ICONS ${babelfish_ICONS} DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)
