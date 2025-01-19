add_definitions(-DTRANSLATION_DOMAIN=\"temporarysavedir\")

set(temporarysavedir_PART_SRCS temporarysavedir.cpp )

add_library(temporarysavedir MODULE ${temporarysavedir_PART_SRCS})

target_link_libraries(temporarysavedir KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts KF${KF_MAJOR_VERSION}::Konq)

konqueror_install_plugin_in_namespaces(temporarysavedir)
