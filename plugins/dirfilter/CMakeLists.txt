############### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"dirfilterplugin\")
set(dirfilterplugin_PART_SRCS dirfilterplugin.cpp )

add_library(dirfilterplugin MODULE ${dirfilterplugin_PART_SRCS})

kcoreaddons_desktop_to_json(dirfilterplugin dirfilterplugin.desktop)
target_link_libraries(dirfilterplugin KF5::I18n KF5::Parts KF5Konq)

konqueror_install_plugin_in_namespaces(dirfilterplugin)
