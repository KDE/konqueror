



########### next target ###############

set(autorefresh_PART_SRCS autorefresh.cpp )

kde4_add_plugin(autorefresh ${autorefresh_PART_SRCS})



target_link_libraries(autorefresh  ${KDE4_KPARTS_LIBS} )

install(TARGETS autorefresh  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES autorefresh.rc autorefresh.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES autorefresh.rc autorefresh.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart/kpartplugins )
