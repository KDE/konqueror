############### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"dirfilterplugin\")
set(dirfilterplugin_PART_SRCS dirfilterplugin.cpp )

add_library(dirfilterplugin MODULE ${dirfilterplugin_PART_SRCS})

target_link_libraries(dirfilterplugin KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts KF${KF_MAJOR_VERSION}Konq)

install(FILES dirfilterplugin.rc DESTINATION ${KDE_INSTALL_DATADIR}/konqueror/partsrcfiles)
install(TARGETS dirfilterplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/dolphinpart/kpartplugins)
