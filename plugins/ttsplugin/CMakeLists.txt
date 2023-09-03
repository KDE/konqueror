########### khtmlttsplugin ###############
add_definitions(-DTRANSLATION_DOMAIN=\"khtmltts\")
set(khtmlttsplugin_PART_SRCS khtmltts.cpp )

add_library(khtmlttsplugin MODULE ${khtmlttsplugin_PART_SRCS})

target_link_libraries(khtmlttsplugin KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts Qt${KF_MAJOR_VERSION}::::TextToSpeech KF${KF_MAJOR_VERSION}::Konq)

konqueror_install_plugin_in_namespaces(khtmlttsplugin)
