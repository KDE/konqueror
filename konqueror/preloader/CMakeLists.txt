include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../src ${CMAKE_CURRENT_BINARY_DIR}/../src )


########### next target ###############
add_definitions(-DKONQPRIVATE_EXPORT=)

set(kded_konqy_preloader_PART_SRCS preloader.cc )

kconfig_add_kcfg_files(kded_konqy_preloader_PART_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../src/konqsettingsxt.kcfgc)

qt5_add_dbus_adaptor(kded_konqy_preloader_PART_SRCS org.kde.konqueror.Preloader.xml preloader.h KonqyPreloader)


add_library(kded_konqy_preloader MODULE ${kded_konqy_preloader_PART_SRCS})

target_link_libraries(kded_konqy_preloader )

install(TARGETS kded_konqy_preloader DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install(FILES konqy_preload.desktop DESTINATION ${AUTOSTART_INSTALL_DIR})
install(FILES konqy_preloader.desktop DESTINATION ${SERVICES_INSTALL_DIR}/kded)
install(FILES org.kde.konqueror.Preloader.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

