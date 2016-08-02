include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../            # sidebar dir
  ${CMAKE_CURRENT_SOURCE_DIR}/../      # trees dir
  ${CMAKE_CURRENT_BINARY_DIR}/../            # trees dir
)

set(konq_sidebartree_bookmarks_SRCS bookmark_module.cpp bookmark_item.cpp ${libkonq_sidebar_tree_SRCS})

add_library(konq_sidebartree_bookmarks MODULE ${konq_sidebartree_bookmarks_SRCS})

target_link_libraries(konq_sidebartree_bookmarks   KF5::Parts konqsidebarplugin konq ${KDE4_KDE3SUPPORT_LIBS})

install(TARGETS konq_sidebartree_bookmarks  DESTINATION ${KDE_INSTALL_PLUGINDIR} )

