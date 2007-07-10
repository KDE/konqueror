include_directories(
  ${CMAKE_SOURCE_DIR}/libkonq 
  ${CMAKE_CURRENT_SOURCE_DIR}/../.. # sidebar dir
  ${CMAKE_CURRENT_SOURCE_DIR}/..    # trees dir
 )


########### next target ###############

set(konq_sidebartree_dirtree_SRCS dirtree_module.cpp dirtree_item.cpp ${libkonq_sidebar_tree_SRCS} )


kde4_add_plugin(konq_sidebartree_dirtree ${konq_sidebartree_dirtree_SRCS})

target_link_libraries(konq_sidebartree_dirtree  ${KDE4_KDE3SUPPORT_LIBS} konq konqsidebarplugin  )

install(TARGETS konq_sidebartree_dirtree  DESTINATION ${PLUGIN_INSTALL_DIR} )


