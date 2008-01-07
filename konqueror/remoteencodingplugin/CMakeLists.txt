########### next target ###############

set(konq_remoteencoding_PART_SRCS kremoteencodingplugin.cpp )


kde4_add_plugin(konq_remoteencoding ${konq_remoteencoding_PART_SRCS})


target_link_libraries(konq_remoteencoding  ${KDE4_KPARTS_LIBS} )

install(TARGETS konq_remoteencoding  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kremoteencodingplugin.rc kremoteencodingplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqlistview/kpartplugins )
install( FILES kremoteencodingplugin.rc kremoteencodingplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqiconview/kpartplugins )

