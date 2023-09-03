########### konqsidebar_places module ###############

set(konqsidebar_places_SRCS
  places_module.cpp
)

add_library(konqsidebar_places MODULE ${konqsidebar_places_SRCS})

target_link_libraries(konqsidebar_places KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts konqsidebarplugin)

install(TARGETS konqsidebar_places DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### install files ###############

install(FILES konqsidebar_places.desktop DESTINATION ${KDE_INSTALL_DATADIR}/konqsidebartng/plugins)
