add_definitions(-DTRANSLATION_DOMAIN=\"kcmkonq\")
set(kcm_konq_PART_SRCS
   behaviour.cpp
   konqkcmfactory.cpp
#    kcustommenueditor.cpp
)

add_library(kcm_konq MODULE ${kcm_konq_PART_SRCS})


target_link_libraries(kcm_konq KF5::KCMUtils KF5::KIOCore KF5::I18n KF5::KIOWidgets Qt5::DBus)

install(TARGETS kcm_konq  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############
install( FILES
   filebehavior.desktop
   #fileappearance.desktop
   DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES overlapping.png onlyone.png  DESTINATION  ${KDE_INSTALL_DATADIR}/kcontrol/pics )

