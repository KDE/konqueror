include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src # konqueror dir (for konqhistorymanager.h)
  ${CMAKE_CURRENT_SOURCE_DIR}/../../ # sidebar dir
  ${CMAKE_CURRENT_SOURCE_DIR}/../    # trees dir
  ${CMAKE_CURRENT_BINARY_DIR}/../            # trees dir
)


########### next target ###############

# dummy module
kde4_add_plugin(konq_sidebartree_history dummy.cpp)
target_link_libraries(konq_sidebartree_history ${QT_QT3SUPPORT_LIBRARY} ${QT_QTCORE_LIBRARY})
install(TARGETS konq_sidebartree_history  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kcm_history_SRCS kcmhistory.cpp ../../../src/konqhistorysettings.cpp)


kde4_add_ui_files(kcm_history_SRCS history_dlg.ui )

kde4_add_plugin(kcm_history ${kcm_history_SRCS})

target_link_libraries(kcm_history konquerorprivate konq ${KDE4_KPARTS_LIBS} ${KDE4_KDEUI_LIBS} )

install(TARGETS kcm_history  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmhistory.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

