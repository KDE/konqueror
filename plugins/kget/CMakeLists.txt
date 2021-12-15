add_library(konqueror_kget_browser_integration MODULE)

set(kget_browser_integration_dbus_SRCS)
qt_add_dbus_interface(kget_browser_integration_dbus_SRCS org.kde.kget.main.xml kget_interface )

target_sources(konqueror_kget_browser_integration PRIVATE
   kget_plugin.cpp
   ${kget_browser_integration_dbus_SRCS}
)

target_link_libraries(konqueror_kget_browser_integration KF5::CoreAddons KF5::I18n KF5::IconThemes KF5Konq)
target_compile_definitions(konqueror_kget_browser_integration PRIVATE TRANSLATION_DOMAIN="kgetplugin")

install(TARGETS konqueror_kget_browser_integration DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### install files ###############
install(FILES kget_plugin.rc kget_plugin.desktop DESTINATION ${KDE_INSTALL_DATADIR}/khtml/kpartplugins)
install(FILES kget_plugin.rc kget_plugin.desktop DESTINATION ${KDE_INSTALL_DATADIR}/dolphinpart/kpartplugins)
install(FILES kget_plugin.rc kget_plugin.desktop DESTINATION ${KDE_INSTALL_DATADIR}/kwebkitpart/kpartplugins)

