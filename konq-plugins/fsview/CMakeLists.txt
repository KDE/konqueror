
add_subdirectory(tests)

########### next target ###############

set(libfsview_SRCS
    treemap.cpp
    fsview.cpp
    scan.cpp
    inode.cpp
    )


########### next target ###############

set(fsview_SRCS main.cpp ${libfsview_SRCS} )

kde4_add_executable(fsview ${fsview_SRCS})

target_link_libraries(fsview  ${KDE4_KIO_LIBS})

install(TARGETS fsview ${INSTALL_TARGETS_DEFAULT_ARGS})


########### next target ###############

set(fsviewpart_PART_SRCS fsview_part.cpp ${libfsview_SRCS})

kde4_add_plugin(fsviewpart  ${fsviewpart_PART_SRCS})

target_link_libraries(fsviewpart  ${KDE4_KPARTS_LIBS}  konq )

install(TARGETS fsviewpart  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES fsview_part.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES fsview_part.rc  DESTINATION  ${DATA_INSTALL_DIR}/fsview )

kde4_install_icons( ${ICON_INSTALL_DIR}   )

