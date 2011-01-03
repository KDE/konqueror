########### next target ###############

set(uachangerplugin_PART_SRCS uachangerplugin.cpp )

kde4_add_plugin(uachangerplugin  ${uachangerplugin_PART_SRCS})

target_link_libraries(uachangerplugin  ${KDE4_KPARTS_LIBS} )

install(TARGETS uachangerplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES uachangerplugin.rc uachangerplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES uachangerplugin.rc uachangerplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart/kpartplugins )
