include_directories(
  ${CMAKE_SOURCE_DIR}/libkonq
  ${CMAKE_CURRENT_SOURCE_DIR}/../.. # sidebar dir
  ${CMAKE_CURRENT_SOURCE_DIR}/..    # trees dir
)


########### next target ###############

set(konq_sidebartree_history_SRCS history_module.cpp history_item.cpp history_settings.cpp ${libkonq_sidebar_tree_SRCS} )

kde4_automoc(${konq_sidebartree_history_SRCS})

kde4_add_plugin(konq_sidebartree_history ${konq_sidebartree_history_SRCS})

target_link_libraries(konq_sidebartree_history  ${KDE4_KDEUI_LIBS} konqsidebarplugin konq )

install(TARGETS konq_sidebartree_history  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(kcm_history_SRCS kcmhistory.cpp history_settings.cpp )

kde4_automoc(${kcm_history_SRCS})

kde4_add_ui_files(kcm_history_SRCS history_dlg.ui )

kde4_add_plugin(kcm_history ${kcm_history_SRCS})

target_link_libraries(kcm_history  ${KDE4_KDEUI_LIBS} konq )

install(TARGETS kcm_history  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmhistory.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

