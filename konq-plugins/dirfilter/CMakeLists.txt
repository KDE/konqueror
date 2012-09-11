########### next target ###############

if (${KDE_VERSION} VERSION_GREATER "4.9.1")

set(dirfilterplugin_PART_SRCS dirfilterplugin.cpp )

kde4_add_plugin(dirfilterplugin  ${dirfilterplugin_PART_SRCS})

target_link_libraries(dirfilterplugin ${KDE4_KPARTS_LIBS})

install(TARGETS dirfilterplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES dirfilterplugin.rc dirfilterplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/dolphinpart/kpartplugins )

endif (${KDE_VERSION} VERSION_GREATER "4.9.1")
