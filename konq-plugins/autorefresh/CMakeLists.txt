



########### next target ###############

set(autorefresh_PART_SRCS autorefresh.cpp )

add_library(autorefresh MODULE ${autorefresh_PART_SRCS})



target_link_libraries(autorefresh  KF5::Parts )

install(TARGETS autorefresh  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES autorefresh.rc autorefresh.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES autorefresh.rc autorefresh.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart/kpartplugins )
