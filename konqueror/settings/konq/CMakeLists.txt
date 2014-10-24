add_definitions(-DTRANSLATION_DOMAIN=\"kcmkonq\")
set(kcm_konq_PART_SRCS
   behaviour.cpp
   konqkcmfactory.cpp
#    kcustommenueditor.cpp
)

add_library(kcm_konq MODULE ${kcm_konq_PART_SRCS})


target_link_libraries(kcm_konq KF5::KCMUtils KF5::KIOCore KF5::I18n KF5::KIOWidgets Qt5::DBus)

install(TARGETS kcm_konq  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############
install( FILES
   filebehavior.desktop
   #fileappearance.desktop
   DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES overlapping.png onlyone.png  DESTINATION  ${DATA_INSTALL_DIR}/kcontrol/pics )

