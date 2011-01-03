set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${KDE4_ENABLE_EXCEPTIONS}")
########### next target ###############

set(webarchiverplugin_PART_SRCS plugin_webarchiver.cpp archivedialog.cpp )

kde4_add_ui_files(webarchiverplugin_PART_SRCS archiveviewbase.ui )

kde4_add_plugin(webarchiverplugin  ${webarchiverplugin_PART_SRCS})



target_link_libraries(webarchiverplugin ${KDE4_KHTML_LIBS})

install(TARGETS webarchiverplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(webarchivethumbnail_PART_SRCS webarchivecreator.cpp )

kde4_add_plugin(webarchivethumbnail ${webarchivethumbnail_PART_SRCS})



target_link_libraries(webarchivethumbnail  ${KDE4_KHTML_LIBS} )

install(TARGETS webarchivethumbnail  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_webarchiver.rc plugin_webarchiver.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES webarchivethumbnail.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR}   )

