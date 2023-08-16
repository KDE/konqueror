



########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"autorefresh\")

set(autorefresh_PART_SRCS autorefresh.cpp )

add_library(autorefresh MODULE ${autorefresh_PART_SRCS})

target_link_libraries(autorefresh KF5::I18n KF5::Parts KF5::IconThemes KF5Konq)

konqueror_install_plugin_in_namespaces(autorefresh)
