
if (X11_Xt_LIB)

  add_subdirectory( viewer ) 
  set(nsplugins_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/plugin_part.cpp ${CMAKE_CURRENT_SOURCE_DIR}/nspluginloader.cpp )


  ########### next target ###############

  set(kcminit_nsplugin_PART_SRCS 
	${nsplugins_SRCS}
     )

  qt4_add_dbus_adaptor( kcminit_nsplugin_PART_SRCS org.kde.nsplugins.CallBack.xml ${CMAKE_CURRENT_SOURCE_DIR}/plugin_part.h PluginPart )

  set(nsplugins_class_xml  ${CMAKE_CURRENT_SOURCE_DIR}/viewer/org.kde.nsplugins.class.xml)
  QT4_ADD_DBUS_INTERFACE( kcminit_nsplugin_PART_SRCS ${nsplugins_class_xml} nsplugins_class_interface )

  set(nsplugins_instance_xml  ${CMAKE_CURRENT_SOURCE_DIR}/viewer/org.kde.nsplugins.instance.xml)
  QT4_ADD_DBUS_INTERFACE( kcminit_nsplugin_PART_SRCS ${nsplugins_instance_xml} nsplugins_instance_interface )

  set(nsplugins_viewer_xml  ${CMAKE_CURRENT_SOURCE_DIR}/viewer/org.kde.nsplugins.viewer.xml)
  QT4_ADD_DBUS_INTERFACE( kcminit_nsplugin_PART_SRCS ${nsplugins_viewer_xml} nsplugins_viewer_interface )




  kde4_add_plugin(kcminit_nsplugin WITH_PREFIX ${kcminit_nsplugin_PART_SRCS})

  target_link_libraries(kcminit_nsplugin  ${KDE4_KPARTS_LIBS} ${QT_QT3SUPPORT_LIBRARY} ${QT_QTGUI_LIBRARY} )

  install(TARGETS kcminit_nsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


  ########### next target ###############

  set(kcm_nsplugins_PART_SRCS kcm_nsplugins.cpp )


  kde4_add_plugin(kcm_nsplugins ${kcm_nsplugins_PART_SRCS})


  target_link_libraries(kcm_nsplugins  ${KDE4_KDECORE_LIBS} )

  install(TARGETS kcm_nsplugins  DESTINATION ${PLUGIN_INSTALL_DIR} )


  ########### next target ###############

  set(nspluginscan_SRCS pluginscan.cpp )


  kde4_add_executable(nspluginscan ${nspluginscan_SRCS})

  target_link_libraries(nspluginscan  ${KDE4_KIO_LIBS} ${X11_Xt_LIB})

  install(TARGETS nspluginscan DESTINATION ${BIN_INSTALL_DIR})

  ########### install files ###############

  install( FILES khtml_plugins.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
  install( FILES nspluginpart.rc  DESTINATION  ${DATA_INSTALL_DIR}/plugin )
  install( FILES org.kde.nsplugins.CallBack.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

  add_subdirectory( test )

else (X11_Xt_LIB)

  message(STATUS "libXt not found - browser plugin support disabled")

endif (X11_Xt_LIB)




