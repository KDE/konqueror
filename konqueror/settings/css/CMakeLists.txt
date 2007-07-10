


########### next target ###############

set(kcm_css_PART_SRCS template.cpp kcmcss.cpp )



kde4_add_ui_files(kcm_css_PART_SRCS csscustom.ui cssconfig.ui preview.ui)

kde4_add_plugin(kcm_css ${kcm_css_PART_SRCS})


target_link_libraries(kcm_css  ${KDE4_KIO_LIBS} ${QT_QT3SUPPORT_LIBRARY} )

install(TARGETS kcm_css  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmcss.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES template.css  DESTINATION  ${DATA_INSTALL_DIR}/kcmcss )

