# !?!?!? make a lib, or clean this up...
set(libkonq_sidebar_tree_SRCS 
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartree.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartreeitem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/konq_sidebartreetoplevelitem.cpp
)

add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

add_subdirectory( init )
add_subdirectory( dirtree_module )
add_subdirectory( bookmark_module )

########### next target ###############

set(konqsidebar_tree_PART_SRCS konqsidebar_oldtreemodule.cpp ${libkonq_sidebar_tree_SRCS})

kde4_add_plugin(konqsidebar_tree ${konqsidebar_tree_PART_SRCS})

target_link_libraries(konqsidebar_tree ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KFILE_LIBS} konq konqsidebarplugin )

install(TARGETS konqsidebar_tree DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install desktop files ###############

install( FILES konqsidebar_tree.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/plugins )

