set(kcm_konq_PART_SRCS
   behaviour.cpp
   globalpaths.cpp
   konqkcmfactory.cpp
#    kcustommenueditor.cpp
)

kde4_add_plugin(kcm_konq ${kcm_konq_PART_SRCS})


target_link_libraries(kcm_konq ${KDE4_KUTILS_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS kcm_konq  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############
install( FILES
   desktoppath.desktop
   filebehavior.desktop
   #fileappearance.desktop
   DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES overlapping.png onlyone.png  DESTINATION  ${DATA_INSTALL_DIR}/kcontrol/pics )

