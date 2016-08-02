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

add_library(konqsidebar_tree MODULE ${konqsidebar_tree_PART_SRCS})

target_link_libraries(konqsidebar_tree ${KDE4_KDE3SUPPORT_LIBS} KF5::Parts  KF5::Konq konqsidebarplugin )

install(TARGETS konqsidebar_tree DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### install desktop files ###############

install( FILES konqsidebar_tree.desktop DESTINATION ${KDE_INSTALL_DATADIR}/konqsidebartng/plugins )

