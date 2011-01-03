########### next target ###############

set(kimgallery_PART_SRCS imgalleryplugin.cpp imgallerydialog.cpp )

kde4_add_plugin(kimgallery  ${kimgallery_PART_SRCS})



target_link_libraries(kimgallery  ${KDE4_KPARTS_LIBS} )

install(TARGETS kimgallery  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kimgalleryplugin.rc kimgalleryplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/dolphinpart/kpartplugins )

kde4_install_icons( ${ICON_INSTALL_DIR}   )


