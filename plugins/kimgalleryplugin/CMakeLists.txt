########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"imgalleryplugin\")
add_definitions(-DKDE_VERSION_STRING=\"${RELEASE_SERVICE_VERSION}\")
set(kimgallery_PART_SRCS imgalleryplugin.cpp imgallerydialog.cpp )
ecm_qt_declare_logging_category(kimgallery_PART_SRCS HEADER imgallery_debug.h IDENTIFIER IMAGEGALLERY_LOG CATEGORY_NAME org.kde.konqueror.imagegalleryplugin)

add_library(kimgallery MODULE ${kimgallery_PART_SRCS})

target_link_libraries(kimgallery KF5::I18n KF5::WidgetsAddons KF5::KIOWidgets KF5::Parts KF5Konq)

install(TARGETS kimgallery  DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### install files ###############

install( FILES kimgalleryplugin.rc kimgalleryplugin.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/dolphinpart/kpartplugins )

set(kimagegallery_ICONS 16-actions-imagegallery.png 22-actions-imagegallery.png)

ecm_install_icons(ICONS ${kimagegallery_ICONS} DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor)
