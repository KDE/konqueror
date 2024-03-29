add_definitions(-DTRANSLATION_DOMAIN=\"webarchiver\")
find_package(KF${KF_MAJOR_VERSION} REQUIRED COMPONENTS CoreAddons Parts)
ecm_qt_declare_logging_category(webarchiverplugin_DEBUG_SRCS HEADER webarchiverdebug.h IDENTIFIER WEBARCHIVERPLUGIN_LOG CATEGORY_NAME org.kde.konqueror.webarchiver)

########### next target ###############

set(webarchiverplugin_PART_SRCS plugin_webarchiver.cpp ${webarchiverplugin_DEBUG_SRCS})
add_library(webarchiverplugin MODULE ${webarchiverplugin_PART_SRCS})
target_include_directories(webarchiverplugin PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(webarchiverplugin KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Widgets KF${KF_MAJOR_VERSION}::Konq)

konqueror_install_plugin_in_namespaces(webarchiverplugin)

ecm_install_icons(
    ICONS 16-actions-webarchiver.png 22-actions-webarchiver.png
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

########### subdirectories ###############

add_subdirectory(app)
add_subdirectory(thumbnailer)
