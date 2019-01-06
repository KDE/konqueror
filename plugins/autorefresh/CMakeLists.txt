



########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"autorefresh\")

set(autorefresh_PART_SRCS autorefresh.cpp )

add_library(autorefresh MODULE ${autorefresh_PART_SRCS})



target_link_libraries(autorefresh  KF5::Parts KF5::IconThemes )

install(TARGETS autorefresh  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES autorefresh.rc autorefresh.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )
install( FILES autorefresh.rc autorefresh.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/kwebkitpart/kpartplugins )
install( FILES autorefresh.rc autorefresh.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/webenginepart/kpartplugins )
