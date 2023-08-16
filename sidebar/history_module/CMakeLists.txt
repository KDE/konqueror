add_definitions(-DTRANSLATION_DOMAIN=\"konqueror\")
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src # konqueror dir (for konqhistory*.h)
  ${CMAKE_CURRENT_SOURCE_DIR}/.. # sidebar dir
)

########### the sidebar module ###############

add_library(konqsidebar_history MODULE history_module.cpp)
target_link_libraries(konqsidebar_history konqsidebarplugin konquerorprivate KF5::Konq KF5::Parts)
install(TARGETS konqsidebar_history DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES konqsidebar_history.desktop DESTINATION ${KDE_INSTALL_DATADIR}/konqsidebartng/plugins)

########### the KCM for the history ###############

set(kcm_history_SRCS kcmhistory.cpp ../../src/konqhistorysettings.cpp)
ki18n_wrap_ui(kcm_history_SRCS history_dlg.ui )
kcoreaddons_add_plugin(kcm_history SOURCES ${kcm_history_SRCS} INSTALL_NAMESPACE konqueror_kcms)

# konquerorprivate is only needed for konqhistorysettings...
target_link_libraries(kcm_history konquerorprivate KF5::Konq KF5::Parts)
