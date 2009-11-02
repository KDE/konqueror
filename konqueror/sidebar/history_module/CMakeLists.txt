include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src # konqueror dir (for konqhistory*.h)
  ${CMAKE_CURRENT_SOURCE_DIR}/.. # sidebar dir
)

########### the sidebar module ###############

kde4_add_plugin(konqsidebar_history history_module.cpp)
target_link_libraries(konqsidebar_history konqsidebarplugin konquerorprivate konq ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS})
install(TARGETS konqsidebar_history DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES konqsidebar_history.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/plugins)

########### the KCM for the history ###############

set(kcm_history_SRCS kcmhistory.cpp ../../src/konqhistorysettings.cpp)
kde4_add_ui_files(kcm_history_SRCS history_dlg.ui )
kde4_add_plugin(kcm_history ${kcm_history_SRCS})

# konquerorprivate is only needed for konqhistorysettings...
target_link_libraries(kcm_history konquerorprivate konq ${KDE4_KPARTS_LIBS} ${KDE4_KDEUI_LIBS} )

install(TARGETS kcm_history DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kcmhistory.desktop DESTINATION ${SERVICES_INSTALL_DIR})

