########### konqsidebar_bookmark module ###############

set(konqsidebar_bookmarks_SRCS
  bookmarks_module.cpp
)

add_library(konqsidebar_bookmarks MODULE ${konqsidebar_bookmarks_SRCS})

target_link_libraries(konqsidebar_bookmarks KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts KF${KF_MAJOR_VERSION}::Konq konqsidebarplugin )

install(TARGETS konqsidebar_bookmarks DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### install files ###############

install(FILES konqsidebar_bookmarks.desktop DESTINATION ${KDE_INSTALL_DATADIR}/konqsidebartng/plugins)
