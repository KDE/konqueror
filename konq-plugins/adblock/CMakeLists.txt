

########### next target ###############

set(adblock_PART_SRCS adblock.cpp  adblockdialog.cpp )

kde4_add_plugin(adblock ${adblock_PART_SRCS})



target_link_libraries(adblock  ${KDE4_KPARTS_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KUTILS_LIBS})

install(TARGETS adblock  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_adblock.desktop  plugin_adblock.rc  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
