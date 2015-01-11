########### next target ###############

set(kimgallery_PART_SRCS imgalleryplugin.cpp imgallerydialog.cpp )

add_library(kimgallery MODULE ${kimgallery_PART_SRCS})



target_link_libraries(kimgallery  KF5::Parts )

install(TARGETS kimgallery  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kimgalleryplugin.rc kimgalleryplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/dolphinpart/kpartplugins )

ecm_install_icons( ${ICON_INSTALL_DIR}   )


