
########### next target ###############

set(konq_shellcmdplugin_PART_SRCS kshellcmdexecutor.cpp kshellcmddialog.cpp kshellcmdplugin.cpp )


add_library(konq_shellcmdplugin MODULE ${konq_shellcmdplugin_PART_SRCS})


target_link_libraries(konq_shellcmdplugin KF5::Parts ${KDE4_KDESU_LIBS})

install(TARGETS konq_shellcmdplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############
install( FILES kshellcmdplugin.rc kshellcmdplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/dolphinpart/kpartplugins )
