add_definitions(-DTRANSLATION_DOMAIN=\"webarchiver\")
find_package(KF5 REQUIRED COMPONENTS KIO Archive KHtml)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${KDE4_ENABLE_EXCEPTIONS}")
########### next target ###############

set(webarchiverplugin_PART_SRCS plugin_webarchiver.cpp archivedialog.cpp )

ecm_qt_declare_logging_category(webarchiverplugin_PART_SRCS HEADER webarchiverdebug.h IDENTIFIER WEBARCHIVERPLUGIN_LOG CATEGORY_NAME org.kde.konqueror.webarchiver)
ki18n_wrap_ui(webarchiverplugin_PART_SRCS archiveviewbase.ui )

add_library(webarchiverplugin MODULE ${webarchiverplugin_PART_SRCS})

target_link_libraries(webarchiverplugin KF5::KHtml KF5::KDELibs4Support KF5::KIOCore KF5::Archive)
install(TARGETS webarchiverplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### next target ###############

set(webarchivethumbnail_PART_SRCS webarchivecreator.cpp )

add_library(webarchivethumbnail MODULE ${webarchivethumbnail_PART_SRCS})

target_link_libraries(webarchivethumbnail KF5::KHtml KF5::KIOCore)

install(TARGETS webarchivethumbnail  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES plugin_webarchiver.rc plugin_webarchiver.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )
install( FILES webarchivethumbnail.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )

ecm_install_icons(
    ICONS
        16-actions-webarchiver.png
        22-actions-webarchiver.png
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

