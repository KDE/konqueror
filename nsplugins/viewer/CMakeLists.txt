include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)

if (GLIB2_FOUND)
  include_directories(${GLIB2_INCLUDE_DIR})
endif (GLIB2_FOUND)

########### next target ###############

set(nspluginviewer_SRCS nsplugin.cpp viewer.cpp xtevents.cpp glibevents.cpp
    pluginhost_xembed.cpp pluginhost_xt.cpp pluginhost.cpp scripting.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../comm/dbustypes.cpp)

qt5_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.class.xml nsplugin.h NSPluginClass )
qt5_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.instance.xml nsplugin.h  NSPluginInstance )
qt5_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.viewer.xml nsplugin.h NSPluginViewer )

set(nsplugins_callback_xml  ${CMAKE_CURRENT_SOURCE_DIR}/../org.kde.nsplugins.CallBack.xml)
qt5_add_dbus_interface( nspluginviewer_SRCS ${nsplugins_callback_xml} nsplugins_callback_interface )


add_executable(nspluginviewer ${nspluginviewer_SRCS})

target_link_libraries(nspluginviewer KF5::Parts ${X11_Xt_LIB} ${X11_X11_LIB} Qt5::DBus ${CMAKE_DL_LIBS})
if (GLIB2_FOUND)
   target_link_libraries(nspluginviewer ${GLIB2_LIBRARIES})
endif (GLIB2_FOUND)


install(TARGETS nspluginviewer ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.nsplugins.class.xml org.kde.nsplugins.instance.xml org.kde.nsplugins.viewer.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )
