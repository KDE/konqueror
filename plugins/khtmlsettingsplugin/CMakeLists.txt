



########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"khtmlsettingsplugin\")

set(khtmlsetting_VERSION 3.3)

configure_file(khtmlsettingsplugin.json.in ${CMAKE_CURRENT_BINARY_DIR}/khtmlsettingsplugin.json)

set(khtmlsettingsplugin_PART_SRCS settingsplugin.cpp )

add_library(khtmlsettingsplugin MODULE ${khtmlsettingsplugin_PART_SRCS})


target_link_libraries(khtmlsettingsplugin KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts KF${KF_MAJOR_VERSION}Konq)

konqueror_install_plugin_in_namespaces(khtmlsettingsplugin)
