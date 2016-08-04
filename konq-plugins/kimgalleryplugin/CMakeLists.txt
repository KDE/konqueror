########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"imgalleryplugin\")
set(kimgallery_PART_SRCS imgalleryplugin.cpp imgallerydialog.cpp )

add_library(kimgallery MODULE ${kimgallery_PART_SRCS})



target_link_libraries(kimgallery  KF5::Parts KF5::KDELibs4Support)

install(TARGETS kimgallery  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES kimgalleryplugin.rc kimgalleryplugin.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/dolphinpart/kpartplugins )

set(kimagegallery_ICONS 16-actions-imagegallery.png 22-actions-imagegallery.png)

ecm_install_icons(ICONS ${kimagegallery_ICONS} DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)


