
########### next target ###############

set(konqsidebar_web_PART_SRCS web_module.cpp )

qt4_add_dbus_interface(konqsidebar_web_PART_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/konq/favicons/org.kde.FavIcon.xml favicon_interface)

kde4_add_plugin(konqsidebar_web ${konqsidebar_web_PART_SRCS})


target_link_libraries(konqsidebar_web  ${KDE4_KHTML_LIBS} konqsidebarplugin konq )

install(TARGETS konqsidebar_web  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES websidebar.html  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/websidebar )
install( FILES webmodule_add.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/add )


