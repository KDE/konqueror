
########### next target ###############
find_package(KF5 REQUIRED CoreAddons Su)
add_definitions(-DTRANSLATION_DOMAIN=\"kshellcmdplugin\")
set(konq_shellcmdplugin_PART_SRCS kshellcmdexecutor.cpp kshellcmddialog.cpp kshellcmdplugin.cpp )

add_library(konq_shellcmdplugin MODULE ${konq_shellcmdplugin_PART_SRCS})

target_link_libraries(konq_shellcmdplugin KF5::I18n KF5::Parts KF5::Su KF5Konq)

install(TARGETS konq_shellcmdplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/dolphinpart/kpartplugins)
install(FILES konq_shellcmdplugin.rc DESTINATION ${KDE_INSTALL_DATADIR}/konqueror/partsrcfiles)
