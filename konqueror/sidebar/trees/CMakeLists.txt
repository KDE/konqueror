
set(libkonq_sidebar_tree_SRCS 
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartree.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartreeitem.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartreetoplevelitem.cpp
	)

add_subdirectory( init )
add_subdirectory( dirtree_module )
add_subdirectory( history_module )
add_subdirectory( bookmark_module )

########### next target ###############


########### next target ###############
set(konqsidebar_tree_PART_SRCS konqsidebar_tree.cpp ${libkonq_sidebar_tree_SRCS})


kde4_add_plugin(konqsidebar_tree ${konqsidebar_tree_PART_SRCS})


target_link_libraries(konqsidebar_tree  ${KDE4_KDECORE_LIBS} konq konqsidebarplugin )

install(TARGETS konqsidebar_tree  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############



