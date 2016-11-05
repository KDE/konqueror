add_subdirectory( remote )
add_subdirectory( services )

########### install files ###############

install( FILES dirtree_module.desktop bookmarks_module.desktop DESTINATION ${KDE_INSTALL_DATADIR}/konqsidebartng/dirtree )

