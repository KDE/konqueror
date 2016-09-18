########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"uachangerplugin\")
set(uachangerplugin_PART_SRCS uachangerplugin.cpp )

add_library(uachangerplugin MODULE ${uachangerplugin_PART_SRCS})

target_link_libraries(uachangerplugin  KF5::Parts )

install(TARGETS uachangerplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### install files ###############

install( FILES uachangerplugin.rc uachangerplugin.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )
install( FILES uachangerplugin.rc uachangerplugin.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/kwebkitpart/kpartplugins )
