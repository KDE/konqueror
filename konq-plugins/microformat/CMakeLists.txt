



########### next target ###############

set(mfkonqmficon_PART_SRCS konqmficon.cpp pluginbase.cpp )

add_library(mfkonqmficon MODULE ${mfkonqmficon_PART_SRCS})



target_link_libraries(mfkonqmficon KF5::KHtml)

install(TARGETS mfkonqmficon  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES microformat.png  DESTINATION  ${DATA_INSTALL_DIR}/microformat/pics )
install( FILES mf_konqmficon.desktop mf_konqmficon.rc  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )

