############### next target ###############

set(dirfilterplugin_PART_SRCS dirfilterplugin.cpp )

add_library(dirfilterplugin MODULE ${dirfilterplugin_PART_SRCS})

target_link_libraries(dirfilterplugin KF5::Parts KF5::KDELibs4Support)

install(TARGETS dirfilterplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES dirfilterplugin.rc dirfilterplugin.desktop  DESTINATION  ${DATA_INSTALL_DIR}/dolphinpart/kpartplugins )
