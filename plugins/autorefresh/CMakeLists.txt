



########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"autorefresh\")

set(autorefresh_PART_SRCS autorefresh.cpp )

add_library(autorefresh MODULE ${autorefresh_PART_SRCS})

target_link_libraries(autorefresh KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts KF${KF_MAJOR_VERSION}::IconThemes KF${KF_MAJOR_VERSION}::Konq)

konqueror_install_plugin_in_namespaces(autorefresh)
