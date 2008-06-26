include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../            # sidebar dir
  ${CMAKE_CURRENT_SOURCE_DIR}/../      # trees dir
  ${CMAKE_CURRENT_BINARY_DIR}/../            # trees dir
)

if(WIN32)
set(konq_sidebartree_bookmarks_SRCS bookmark_module.cpp bookmark_item.cpp)
else(WIN32)
set(konq_sidebartree_bookmarks_SRCS bookmark_module.cpp bookmark_item.cpp ${libkonq_sidebar_tree_SRCS})
endif(WIN32)

kde4_add_plugin(konq_sidebartree_bookmarks ${konq_sidebartree_bookmarks_SRCS})

if(WIN32)
# please remove next lib and add the sources here for 4.1 if cmake > 2.6 is available
target_link_libraries(konq_sidebartree_bookmarks sidebar_tree)
endif(WIN32)
target_link_libraries(konq_sidebartree_bookmarks  ${KDE4_KDEUI_LIBS} konqsidebarplugin konq ${KDE4_KDE3SUPPORT_LIBS})

install(TARGETS konq_sidebartree_bookmarks  DESTINATION ${PLUGIN_INSTALL_DIR} )

