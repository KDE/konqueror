
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

add_executable(fsview ${fsview_SRCS})

target_link_libraries(fsview KF5::KIOWidgets KF5::KDELibs4Support)

install(TARGETS fsview ${INSTALL_TARGETS_DEFAULT_ARGS})


########### next target ###############

set(fsviewpart_PART_SRCS fsview_part.cpp ${libfsview_SRCS})

add_library(fsviewpart MODULE ${fsviewpart_PART_SRCS})

target_link_libraries(fsviewpart KF5::Parts KF5::KDELibs4Support)

install(TARGETS fsviewpart  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES fsview_part.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES fsview_part.rc  DESTINATION  ${DATA_INSTALL_DIR}/fsview )

ecm_install_icons( ${ICON_INSTALL_DIR}   )

