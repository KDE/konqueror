include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../ # sidebar dir
  ${CMAKE_CURRENT_SOURCE_DIR}/../    # trees dir
  ${CMAKE_CURRENT_BINARY_DIR}/../            # trees dir
 )


########### next target ###############

set(konq_sidebartree_dirtree_SRCS dirtree_module.cpp dirtree_item.cpp )


kde4_add_plugin(konq_sidebartree_dirtree ${konq_sidebartree_dirtree_SRCS})

# please remove next lib and add the sources here for 4.1 if cmake > 2.6 is available
target_link_libraries(konq_sidebartree_dirtree sidebar_tree )
target_link_libraries(konq_sidebartree_dirtree  ${KDE4_KDE3SUPPORT_LIBS} konq konqsidebarplugin  )

install(TARGETS konq_sidebartree_dirtree  DESTINATION ${PLUGIN_INSTALL_DIR} )


