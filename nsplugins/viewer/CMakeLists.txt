include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)

if (GLIB2_FOUND)
  include_directories(${GLIB2_INCLUDE_DIR})
endif (GLIB2_FOUND)

########### next target ###############

set(nspluginviewer_SRCS nsplugin.cpp viewer.cpp xtevents.cpp glibevents.cpp
    pluginhost_xembed.cpp pluginhost_xt.cpp pluginhost.cpp)

qt4_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.class.xml nsplugin.h NSPluginClass )
qt4_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.instance.xml nsplugin.h NSPluginInstance )
qt4_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.viewer.xml nsplugin.h NSPluginViewer )

set(nsplugins_callback_xml  ${CMAKE_CURRENT_SOURCE_DIR}/../org.kde.nsplugins.CallBack.xml)
QT4_ADD_DBUS_INTERFACE( nspluginviewer_SRCS ${nsplugins_callback_xml} nsplugins_callback_interface )


kde4_add_executable(nspluginviewer ${nspluginviewer_SRCS})

target_link_libraries(nspluginviewer ${KDE4_KPARTS_LIBS} ${X11_Xt_LIB} ${X11_X11_LIB} ${QT_QTDBUS_LIBRARY} ${CMAKE_DL_LIBS})
if (GLIB2_FOUND)
   target_link_libraries(nspluginviewer ${GLIB2_LIBRARIES})
endif (GLIB2_FOUND)


install(TARGETS nspluginviewer ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.nsplugins.class.xml org.kde.nsplugins.instance.xml org.kde.nsplugins.viewer.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
