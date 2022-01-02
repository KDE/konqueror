########### khtmlttsplugin ###############
add_definitions(-DTRANSLATION_DOMAIN=\"khtmltts\")
set(khtmlttsplugin_PART_SRCS khtmltts.cpp )

add_library(khtmlttsplugin MODULE ${khtmlttsplugin_PART_SRCS})

kcoreaddons_desktop_to_json(khtmlttsplugin khtmltts.desktop)
target_link_libraries(khtmlttsplugin KF5::I18n KF5::Parts Qt5::TextToSpeech KF5Konq)

konqueror_install_plugin_in_namespaces(khtmlttsplugin)
