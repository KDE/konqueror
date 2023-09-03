########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"uachangerplugin\")
set(uachangerplugin_PART_SRCS uachangerplugin.cpp )

add_library(uachangerplugin MODULE ${uachangerplugin_PART_SRCS})

target_link_libraries(uachangerplugin KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts KF${KF_MAJOR_VERSION}::Konq)

konqueror_install_plugin_in_namespaces(uachangerplugin)
