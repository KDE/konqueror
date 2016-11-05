



########### next target ###############

set(mfkonqmficon_PART_SRCS konqmficon.cpp pluginbase.cpp )

add_library(mfkonqmficon MODULE ${mfkonqmficon_PART_SRCS})



target_link_libraries(mfkonqmficon KF5::KHtml)

install(TARGETS mfkonqmficon  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES microformat.png  DESTINATION  ${KDE_INSTALL_DATADIR}/microformat/pics )
install( FILES mf_konqmficon.desktop mf_konqmficon.rc  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )

