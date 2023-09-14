########### konqsidebar_tree module ###############

set(konqsidebar_tree_SRCS
  tree_module.cpp
)

add_library(konqsidebar_tree MODULE ${konqsidebar_tree_SRCS})

target_link_libraries(konqsidebar_tree KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Konq konqsidebarplugin )

install(TARGETS konqsidebar_tree DESTINATION ${KDE_INSTALL_PLUGINDIR}/konqueror/sidebar)

########### install files ###############

# install(FILES konqsidebar_tree.desktop DESTINATION ${KDE_INSTALL_DATADIR}/konqsidebartng/plugins)
