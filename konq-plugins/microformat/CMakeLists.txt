



########### next target ###############

set(mfkonqmficon_PART_SRCS konqmficon.cpp pluginbase.cpp )

kde4_add_plugin(mfkonqmficon ${mfkonqmficon_PART_SRCS})



target_link_libraries(mfkonqmficon ${KDE4_KHTML_LIBS})

install(TARGETS mfkonqmficon  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES microformat.png  DESTINATION  ${DATA_INSTALL_DIR}/microformat/pics )
install( FILES mf_konqmficon.desktop mf_konqmficon.rc  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )

