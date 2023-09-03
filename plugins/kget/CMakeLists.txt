add_library(konqueror_kget_browser_integration MODULE)

set(kget_browser_integration_dbus_SRCS)
qt_add_dbus_interface(kget_browser_integration_dbus_SRCS org.kde.kget.main.xml kget_interface )

target_sources(konqueror_kget_browser_integration PRIVATE
   kget_plugin.cpp
   ${kget_browser_integration_dbus_SRCS}
)

target_link_libraries(konqueror_kget_browser_integration KF${KF_MAJOR_VERSION}::CoreAddons KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::IconThemes KF${KF_MAJOR_VERSION}::Konq)
target_compile_definitions(konqueror_kget_browser_integration PRIVATE TRANSLATION_DOMAIN="kgetplugin")

konqueror_install_plugin_in_namespaces(konqueror_kget_browser_integration)
