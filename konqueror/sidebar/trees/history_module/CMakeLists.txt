include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src # konqueror dir (for konqhistorymanager.h)
  ${CMAKE_CURRENT_SOURCE_DIR}/../../ # sidebar dir
  ${CMAKE_CURRENT_SOURCE_DIR}/../    # trees dir
  ${CMAKE_CURRENT_BINARY_DIR}/../            # trees dir
)


########### next target ###############
if(WIN32)
set(konq_sidebartree_history_SRCS history_module.cpp history_item.cpp history_settings.cpp )
else(WIN32)
set(konq_sidebartree_history_SRCS history_module.cpp history_item.cpp history_settings.cpp ${libkonq_sidebar_tree_SRCS} )
endif(WIN32)

kde4_add_plugin(konq_sidebartree_history ${konq_sidebartree_history_SRCS})

if(WIN32)
# please remove next lib and add the sources here for 4.1 if cmake > 2.6 is available
target_link_libraries(konq_sidebartree_history sidebar_tree )
endif(WIN32)
target_link_libraries(konq_sidebartree_history  konqsidebarplugin konquerorprivate konq )

install(TARGETS konq_sidebartree_history  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(kcm_history_SRCS kcmhistory.cpp history_settings.cpp )


kde4_add_ui_files(kcm_history_SRCS history_dlg.ui )

kde4_add_plugin(kcm_history ${kcm_history_SRCS})

target_link_libraries(kcm_history konquerorprivate konq )

install(TARGETS kcm_history  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmhistory.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

