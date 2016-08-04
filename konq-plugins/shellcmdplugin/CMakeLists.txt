
########### next target ###############
find_package(KF5 REQUIRED Su)
add_definitions(-DTRANSLATION_DOMAIN=\"kshellcmdplugin\")
set(konq_shellcmdplugin_PART_SRCS kshellcmdexecutor.cpp kshellcmddialog.cpp kshellcmdplugin.cpp )


add_library(konq_shellcmdplugin MODULE ${konq_shellcmdplugin_PART_SRCS})


target_link_libraries(konq_shellcmdplugin KF5::Parts KF5::Su KF5::KDELibs4Support)

install(TARGETS konq_shellcmdplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############
install( FILES kshellcmdplugin.rc kshellcmdplugin.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/dolphinpart/kpartplugins )
