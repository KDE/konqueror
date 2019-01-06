



########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"khtmlsettingsplugin\")

find_package(KF5 REQUIRED KHtml)

set(khtmlsettingsplugin_PART_SRCS settingsplugin.cpp )

add_library(khtmlsettingsplugin MODULE ${khtmlsettingsplugin_PART_SRCS})



target_link_libraries(khtmlsettingsplugin  KF5::Parts KF5::KHtml)

install(TARGETS khtmlsettingsplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES khtmlsettingsplugin.rc khtmlsettingsplugin.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )
install( FILES khtmlsettingsplugin.rc khtmlsettingsplugin.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/kwebkitpart/kpartplugins )
install( FILES khtmlsettingsplugin.rc khtmlsettingsplugin.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/webenginepart/kpartplugins )

