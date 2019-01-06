


########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"rellinks\")
find_package(KF5 REQUIRED KHtml)

set(rellinksplugin_PART_SRCS plugin_rellinks.cpp )

add_library(rellinksplugin MODULE ${rellinksplugin_PART_SRCS})



target_link_libraries(rellinksplugin  KF5::KHtml KF5::IconThemes)

install(TARGETS rellinksplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES plugin_rellinks.rc plugin_rellinks.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )


