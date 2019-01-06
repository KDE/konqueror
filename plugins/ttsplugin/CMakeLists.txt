########### khtmlttsplugin ###############
add_definitions(-DTRANSLATION_DOMAIN=\"khtmltts\")
set(khtmlttsplugin_PART_SRCS khtmltts.cpp )

add_library(khtmlttsplugin MODULE ${khtmlttsplugin_PART_SRCS})

target_link_libraries(khtmlttsplugin  KF5::Parts Qt5::TextToSpeech)

install(TARGETS khtmlttsplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES khtmltts.rc khtmltts.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )
install( FILES khtmltts.rc khtmltts.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/kwebkitpart/kpartplugins )
install( FILES khtmltts.rc khtmltts.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/webenginepart/kpartplugins )

