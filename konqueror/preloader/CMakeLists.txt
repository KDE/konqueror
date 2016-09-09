include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_CURRENT_BINARY_DIR}/../src )


########### next target ###############
add_definitions(-DKONQPRIVATE_EXPORT=)

set(kded_konqy_preloader_PART_SRCS preloader.cc )

kconfig_add_kcfg_files(kded_konqy_preloader_PART_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../src/konqsettingsxt.kcfgc)

qt5_add_dbus_adaptor(kded_konqy_preloader_PART_SRCS org.kde.konqueror.Preloader.xml preloader.h KonqyPreloader)


add_library(kded_konqy_preloader MODULE ${kded_konqy_preloader_PART_SRCS})

target_link_libraries(kded_konqy_preloader
   KF5::ConfigGui
   KF5::CoreAddons
   KF5::Completion
   KF5::DBusAddons
)

install(TARGETS kded_konqy_preloader DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install(FILES konqy_preload.desktop DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
install(FILES konqy_preloader.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kded)
install(FILES org.kde.konqueror.Preloader.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )

