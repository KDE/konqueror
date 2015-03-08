include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../ # sidebar dir
  ${CMAKE_CURRENT_SOURCE_DIR}/../    # trees dir
  ${CMAKE_CURRENT_BINARY_DIR}/../            # trees dir
 )


########### next target ###############
set(konq_sidebartree_dirtree_SRCS dirtree_module.cpp dirtree_item.cpp ${libkonq_sidebar_tree_SRCS} )

add_library(konq_sidebartree_dirtree MODULE ${konq_sidebartree_dirtree_SRCS})

target_link_libraries(konq_sidebartree_dirtree  ${KDE4_KDE3SUPPORT_LIBS} KF5::Parts konq konqsidebarplugin  )

install(TARGETS konq_sidebartree_dirtree  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


