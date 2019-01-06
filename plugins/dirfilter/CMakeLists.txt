############### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"dirfilterplugin\")
set(dirfilterplugin_PART_SRCS dirfilterplugin.cpp )

add_library(dirfilterplugin MODULE ${dirfilterplugin_PART_SRCS})

target_link_libraries(dirfilterplugin KF5::Parts)

install(TARGETS dirfilterplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES dirfilterplugin.rc dirfilterplugin.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/dolphinpart/kpartplugins )
