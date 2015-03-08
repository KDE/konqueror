########### konqsidebar_web module ###############

set(konqsidebar_web_PART_SRCS web_module.cpp )

qt5_add_dbus_interface(konqsidebar_web_PART_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/konq/favicons/org.kde.FavIcon.xml favicon_interface)

add_library(konqsidebar_web MODULE ${konqsidebar_web_PART_SRCS})

target_link_libraries(konqsidebar_web  KF5::KHtml konqsidebarplugin konq )

install(TARGETS konqsidebar_web  DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### install files ###############

install( FILES konqsidebar_web.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/konqsidebartng/plugins )
