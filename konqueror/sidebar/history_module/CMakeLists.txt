include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src # konqueror dir (for konqhistory*.h)
  ${CMAKE_CURRENT_SOURCE_DIR}/.. # sidebar dir
)

########### the sidebar module ###############

add_library(konqsidebar_history MODULE history_module.cpp)
target_link_libraries(konqsidebar_history konqsidebarplugin konquerorprivate konq  KF5::Parts)
install(TARGETS konqsidebar_history DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES konqsidebar_history.desktop DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/plugins)

########### the KCM for the history ###############

set(kcm_history_SRCS kcmhistory.cpp ../../src/konqhistorysettings.cpp)
ki18n_wrap_ui(kcm_history_SRCS history_dlg.ui )
add_library(kcm_history MODULE ${kcm_history_SRCS})

# konquerorprivate is only needed for konqhistorysettings...
target_link_libraries(kcm_history konquerorprivate KF5::Konq KF5::Parts  )

install(TARGETS kcm_history DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kcmhistory.desktop DESTINATION ${SERVICES_INSTALL_DIR})

