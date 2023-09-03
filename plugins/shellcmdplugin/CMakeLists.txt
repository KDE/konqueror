
########### next target ###############
find_package(KF${KF_MAJOR_VERSION} REQUIRED CoreAddons Su)
add_definitions(-DTRANSLATION_DOMAIN=\"kshellcmdplugin\")
set(konq_shellcmdplugin_PART_SRCS kshellcmdexecutor.cpp kshellcmddialog.cpp kshellcmdplugin.cpp )

add_library(konq_shellcmdplugin MODULE ${konq_shellcmdplugin_PART_SRCS})

target_link_libraries(konq_shellcmdplugin KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts KF${KF_MAJOR_VERSION}::Su KF${KF_MAJOR_VERSION}::Konq)

install(TARGETS konq_shellcmdplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/dolphinpart/kpartplugins)
install(FILES konq_shellcmdplugin.rc DESTINATION ${KDE_INSTALL_DATADIR}/konqueror/partsrcfiles)
