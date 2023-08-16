add_definitions(-DTRANSLATION_DOMAIN=\"webarchiver\")
find_package(KF5 REQUIRED COMPONENTS CoreAddons Parts)

########### next target ###############

set(webarchiverplugin_PART_SRCS plugin_webarchiver.cpp)
add_library(webarchiverplugin MODULE ${webarchiverplugin_PART_SRCS})
target_include_directories(webarchiverplugin PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/logging)
target_link_libraries(webarchiverplugin webarchiverdebug KF5::I18n KF5::Parts Qt5::Core Qt5::Widgets KF5Konq)

konqueror_install_plugin_in_namespaces(webarchiverplugin)

ecm_install_icons(
    ICONS 16-actions-webarchiver.png 22-actions-webarchiver.png
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

########### subdirectories ###############

add_subdirectory(logging)
add_subdirectory(app)
add_subdirectory(thumbnailer)
