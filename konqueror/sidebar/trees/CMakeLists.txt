
set(libkonq_sidebar_tree_SRCS 
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartree.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartreeitem.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartreetoplevelitem.cpp
	)
if(WIN32)
# please remove for 4.1 if cmake > 2.6 is available
add_library(sidebar_tree STATIC ${libkonq_sidebar_tree_SRCS})
endif(WIN32)

add_subdirectory( init )
add_subdirectory( dirtree_module )
add_subdirectory( history_module )
add_subdirectory( bookmark_module )

########### next target ###############


########### next target ###############
set(konqsidebar_tree_PART_SRCS konqsidebar_tree.cpp ${libkonq_sidebar_tree_SRCS})


kde4_add_plugin(konqsidebar_tree ${konqsidebar_tree_PART_SRCS})


target_link_libraries(konqsidebar_tree ${KDE4_KDE3SUPPORT_LIBS} konq konqsidebarplugin )

install(TARGETS konqsidebar_tree  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############



