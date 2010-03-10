########### konqsidebar_places module ###############

set(konqsidebar_places_SRCS
  places_module.cpp
)

kde4_add_plugin(konqsidebar_places ${konqsidebar_places_SRCS})

target_link_libraries(konqsidebar_places ${KDE4_KFILE_LIBS} ${KDE4_KPARTS_LIBRARY} konqsidebarplugin)

install(TARGETS konqsidebar_places DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES konqsidebar_places.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/plugins)
