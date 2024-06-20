add_definitions(-DTRANSLATION_DOMAIN=\"kcmkonq\")

kcoreaddons_add_plugin(kcm_konq SOURCES behaviour.cpp INSTALL_NAMESPACE konqueror_kcms)

target_link_libraries(kcm_konq
  KF${KF_MAJOR_VERSION}::KCMUtils
  KF${KF_MAJOR_VERSION}::KIOCore
  KF${KF_MAJOR_VERSION}::I18n
  KF${KF_MAJOR_VERSION}::KIOWidgets
  Qt${KF_MAJOR_VERSION}::DBus
  KF${KF_MAJOR_VERSION}::KonqSettings)

install( FILES overlapping.png onlyone.png  DESTINATION  ${KDE_INSTALL_DATADIR}/kcontrol/pics )

