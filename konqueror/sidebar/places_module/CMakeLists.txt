########### konqsidebar_places module ###############

set(konqsidebar_places_SRCS
  places_module.cpp
)

add_library(konqsidebar_places MODULE ${konqsidebar_places_SRCS})

target_link_libraries(konqsidebar_places  KF5::Parts konqsidebarplugin)

install(TARGETS konqsidebar_places DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES konqsidebar_places.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/plugins)
