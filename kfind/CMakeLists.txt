set(libkfindcommon_SRCS kftabdlg.cpp kfind.cpp kquery.cpp kdatecombo.cpp)
set(kfindpart_PART_SRCS kfindpart.cpp ${libkfindcommon_SRCS})


kde4_add_plugin(kfindpart WITH_PREFIX ${kfindpart_PART_SRCS})

target_link_libraries(kfindpart ${KDE4_KPARTS_LIBS})

install(TARGETS kfindpart  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(kfind_SRCS kfindtreeview.cpp kfinddlg.cpp main.cpp ${libkfindcommon_SRCS})

kde4_add_app_icon(kfind_SRCS "hi*-app-kfind.png")

kde4_add_executable(kfind ${kfind_SRCS})

target_link_libraries(kfind konq ${KDE4_KFILE_LIBS})

install(TARGETS kfind ${INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install( FILES kfind.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES findpart.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES kfindpart.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR}  )
