include_directories(
  ${CMAKE_SOURCE_DIR}/libkonq
  ${CMAKE_CURRENT_SOURCE_DIR}/../.. # sidebar dir
  ${CMAKE_CURRENT_SOURCE_DIR}/..    # trees dir
)

set(konq_sidebartree_bookmarks_SRCS bookmark_module.cpp bookmark_item.cpp ${libkonq_sidebar_tree_SRCS} )


kde4_add_plugin(konq_sidebartree_bookmarks ${konq_sidebartree_bookmarks_SRCS})

target_link_libraries(konq_sidebartree_bookmarks  ${KDE4_KDEUI_LIBS} konqsidebarplugin konq)

install(TARGETS konq_sidebartree_bookmarks  DESTINATION ${PLUGIN_INSTALL_DIR} )

