

########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"adblock\")
set(adblock_PART_SRCS adblock.cpp  adblockdialog.cpp )

add_library(adblock MODULE ${adblock_PART_SRCS})



target_link_libraries(adblock  KF5::Parts KF5::KHtml KF5::KCMUtils KF5::KDELibs4Support)

install(TARGETS adblock  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES plugin_adblock.desktop  plugin_adblock.rc  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )
