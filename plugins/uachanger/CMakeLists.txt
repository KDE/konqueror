########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"uachangerplugin\")
set(uachangerplugin_PART_SRCS uachangerplugin.cpp )

add_library(uachangerplugin MODULE ${uachangerplugin_PART_SRCS})

kcoreaddons_desktop_to_json(uachangerplugin uachangerplugin.desktop)
target_link_libraries(uachangerplugin KF5::I18n KF5::Parts KF5Konq Qt::WebEngineWidgets kwebenginepartlib)

konqueror_install_plugin_in_namespaces(uachangerplugin)
