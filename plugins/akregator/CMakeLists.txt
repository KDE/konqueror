
ecm_qt_declare_logging_category(akregatorplugin_DEBUG_SRCS HEADER akregatorplugindebug.h IDENTIFIER AKREGATORPLUGIN_LOG CATEGORY_NAME org.kde.konqueror.akregatorplugin)

########### next target ###############

set(akregatorplugin_PART_SRCS akregatorplugin.cpp pluginutil.cpp )

kcoreaddons_add_plugin(akregatorplugin SOURCES ${akregatorplugin_PART_SRCS} ${akregatorplugin_DEBUG_SRCS} INSTALL_NAMESPACE "kf${KF_MAJOR_VERSION}/kfileitemaction")

target_compile_definitions(akregatorplugin PRIVATE TRANSLATION_DOMAIN="akregator_konqplugin")
target_link_libraries(akregatorplugin KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::KIOWidgets Qt${KF_MAJOR_VERSION}::DBus)

########### next target ###############

set(akregatorkonqfeedicon_PART_SRCS konqfeedicon.cpp pluginutil.cpp )

add_library(akregatorkonqfeedicon MODULE ${akregatorkonqfeedicon_PART_SRCS} ${akregatorplugin_DEBUG_SRCS})

target_compile_definitions(akregatorkonqfeedicon PRIVATE TRANSLATION_DOMAIN="akregator_konqplugin")
target_link_libraries(akregatorkonqfeedicon KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::IconThemes KF${KF_MAJOR_VERSION}::Konq)

konqueror_install_plugin_in_namespaces(akregatorkonqfeedicon)

########### install files ###############

install( FILES feed.png DESTINATION ${KDE_INSTALL_DATADIR}/akregator/pics )

# contains list of debug categories, for kdebugsettings
install(FILES akregatorplugin.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
