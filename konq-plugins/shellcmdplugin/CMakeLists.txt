
########### next target ###############

set(konq_shellcmdplugin_PART_SRCS kshellcmdexecutor.cpp kshellcmddialog.cpp kshellcmdplugin.cpp )


kde4_add_plugin(konq_shellcmdplugin ${konq_shellcmdplugin_PART_SRCS})


target_link_libraries(konq_shellcmdplugin ${KDE4_KPARTS_LIBS} ${KDE4_KDESU_LIBS})

install(TARGETS konq_shellcmdplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############
install( FILES kshellcmdplugin.rc kshellcmdplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/dolphinpart/kpartplugins )
