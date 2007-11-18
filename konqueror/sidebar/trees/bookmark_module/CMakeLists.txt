include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../            # sidebar dir
  ${CMAKE_CURRENT_SOURCE_DIR}/../      # trees dir
  ${CMAKE_CURRENT_BINARY_DIR}/../            # trees dir
)

set(konq_sidebartree_bookmarks_SRCS bookmark_module.cpp bookmark_item.cpp)


kde4_add_plugin(konq_sidebartree_bookmarks ${konq_sidebartree_bookmarks_SRCS})

# please remove next lib and add the sources here for 4.1 if cmake > 2.6 is available
target_link_libraries(konq_sidebartree_bookmarks sidebar_tree)
target_link_libraries(konq_sidebartree_bookmarks  ${KDE4_KDEUI_LIBS} konqsidebarplugin konq)

install(TARGETS konq_sidebartree_bookmarks  DESTINATION ${PLUGIN_INSTALL_DIR} )

