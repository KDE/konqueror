

########### next target ###############

set(adblock_PART_SRCS adblock.cpp  adblockdialog.cpp )

add_library(adblock MODULE ${adblock_PART_SRCS})



target_link_libraries(adblock  KF5::Parts KF5::KHtml KF5::KCMUtils)

install(TARGETS adblock  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_adblock.desktop  plugin_adblock.rc  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
