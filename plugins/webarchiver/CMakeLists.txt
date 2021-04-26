add_definitions(-DTRANSLATION_DOMAIN=\"webarchiver\")
find_package(KF5 REQUIRED COMPONENTS CoreAddons Parts)

########### next target ###############

set(webarchiverplugin_PART_SRCS plugin_webarchiver.cpp)
add_library(webarchiverplugin MODULE ${webarchiverplugin_PART_SRCS})
target_include_directories(webarchiverplugin PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/logging)
target_link_libraries(webarchiverplugin webarchiverdebug KF5::I18n KF5::Parts Qt5::Core Qt5::Widgets)

########### install files ###############

install(TARGETS webarchiverplugin DESTINATION ${KDE_INSTALL_PLUGINDIR})

set(plugin_FILES plugin_webarchiver.rc plugin_webarchiver.desktop)
install(FILES ${plugin_FILES} DESTINATION ${KDE_INSTALL_DATADIR}/khtml/kpartplugins)
install(FILES ${plugin_FILES} DESTINATION ${KDE_INSTALL_DATADIR}/kwebkitpart/kpartplugins)
install(FILES ${plugin_FILES} DESTINATION ${KDE_INSTALL_DATADIR}/webenginepart/kpartplugins)

ecm_install_icons(
    ICONS 16-actions-webarchiver.png 22-actions-webarchiver.png
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

########### subdirectories ###############

add_subdirectory(logging)
add_subdirectory(app)
add_subdirectory(thumbnailer)
