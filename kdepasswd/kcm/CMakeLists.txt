
add_subdirectory( pics ) 
add_definitions(-DQT3_SUPPORT)

########### next target ###############

set(kcm_useraccount_PART_SRCS chfnprocess.cpp main.cpp chfacedlg.cpp )


kde4_add_ui_files(kcm_useraccount_PART_SRCS main_widget.ui faceDlg.ui)

kde4_add_kcfg_files(kcm_useraccount_PART_SRCS settings.kcfgc )

kde4_add_plugin(kcm_useraccount ${kcm_useraccount_PART_SRCS})


target_link_libraries(kcm_useraccount  konq ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KDESU_LIBS} )

install(TARGETS kcm_useraccount  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcm_useraccount.desktop   DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kcm_useraccount.kcfg   DESTINATION  ${KCFG_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR}  )


