########### next target ###############

set(kimgallery_PART_SRCS imgalleryplugin.cpp imgallerydialog.cpp )

add_library(kimgallery MODULE ${kimgallery_PART_SRCS})



target_link_libraries(kimgallery  KF5::Parts KF5::KDELibs4Support)

install(TARGETS kimgallery  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kimgalleryplugin.rc kimgalleryplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/dolphinpart/kpartplugins )

set(kimagegallery_ICONS hi16-actions-imagegallery.png hi22-actions-imagegallery.png)

ecm_install_icons(ICONS ${kimagegallery_ICONS} DESTINATION ${ICON_INSTALL_DIR} THEME hicolor)


