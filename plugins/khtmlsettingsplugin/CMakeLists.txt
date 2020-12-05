



########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"khtmlsettingsplugin\")

set(khtmlsetting_VERSION 3.3)

configure_file(khtmlsettingsplugin.desktop ${CMAKE_CURRENT_BINARY_DIR}/khtmlsettingsplugin.desktop)

set(khtmlsettingsplugin_PART_SRCS settingsplugin.cpp )

add_library(khtmlsettingsplugin MODULE ${khtmlsettingsplugin_PART_SRCS})
kcoreaddons_desktop_to_json(khtmlsettingsplugin ${CMAKE_CURRENT_BINARY_DIR}/khtmlsettingsplugin.desktop)


target_link_libraries(khtmlsettingsplugin  KF5::Parts)

install(TARGETS khtmlsettingsplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install(
    FILES khtmlsettingsplugin.rc ${CMAKE_CURRENT_BINARY_DIR}/khtmlsettingsplugin.desktop
    DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins
)
install(
    FILES khtmlsettingsplugin.rc ${CMAKE_CURRENT_BINARY_DIR}/khtmlsettingsplugin.desktop
    DESTINATION  ${KDE_INSTALL_DATADIR}/kwebkitpart/kpartplugins
)
install(
    FILES khtmlsettingsplugin.rc ${CMAKE_CURRENT_BINARY_DIR}/khtmlsettingsplugin.desktop
    DESTINATION  ${KDE_INSTALL_DATADIR}/webenginepart/kpartplugins
)
