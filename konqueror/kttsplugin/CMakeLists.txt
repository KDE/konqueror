


########### next target ###############

set(khtmlkttsdplugin_PART_SRCS khtmlkttsd.cpp )


kde4_add_plugin(khtmlkttsdplugin WITH_PREFIX ${khtmlkttsdplugin_PART_SRCS})


target_link_libraries(khtmlkttsdplugin  ${KDE4_KHTML_LIBS} )

install(TARGETS khtmlkttsdplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
