include_directories( ${CMAKE_SOURCE_DIR}/nsplugins )


########### next target ###############

set(nspluginviewer_SRCS nsplugin.cpp viewer.cpp )

qt4_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.class.xml nsplugin.h NSPluginClass )
qt4_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.instance.xml nsplugin.h NSPluginInstance )
qt4_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.viewer.xml nsplugin.h NSPluginViewer )

set(nsplugins_callback_xml  ${CMAKE_SOURCE_DIR}/nsplugins/org.kde.nsplugins.CallBack.xml)
QT4_ADD_DBUS_INTERFACE( nspluginviewer_SRCS ${nsplugins_callback_xml} nsplugins_callback_interface )

kde4_automoc(${nspluginviewer_SRCS})



kde4_add_executable(nspluginviewer ${nspluginviewer_SRCS})

target_link_libraries(nspluginviewer  ${KDE4_KPARTS_LIBS} ${X11_Xt_LIB} )

install(TARGETS nspluginviewer DESTINATION ${BIN_INSTALL_DIR})
install(FILES org.kde.nsplugins.class.xml org.kde.nsplugins.instance.xml org.kde.nsplugins.viewer.xml DESTINATION ${DBUS_INTERFACES_DIR} )


