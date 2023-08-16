add_definitions(-DTRANSLATION_DOMAIN=\"kcmkonq\")

kcoreaddons_add_plugin(kcm_konq SOURCES behaviour.cpp INSTALL_NAMESPACE konqueror_kcms)

target_link_libraries(kcm_konq KF5::KCMUtils KF5::KIOCore KF5::I18n KF5::KIOWidgets Qt5::DBus)

install( FILES overlapping.png onlyone.png  DESTINATION  ${KDE_INSTALL_DATADIR}/kcontrol/pics )

