include_directories( ${CMAKE_SOURCE_DIR}/nsplugins )


########### next target ###############

set(nspluginviewer_SRCS nsplugin.cpp viewer.cpp callback_proxy.cpp)

qt4_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.class.xml nsplugin.h NSPluginClass )
qt4_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.instance.xml nsplugin.h NSPluginInstance )
qt4_add_dbus_adaptor( nspluginviewer_SRCS org.kde.nsplugins.viewer.xml nsplugin.h NSPluginViewer )



kde4_automoc(${nspluginviewer_SRCS})



kde4_add_executable(nspluginviewer ${nspluginviewer_SRCS})

target_link_libraries(nspluginviewer  ${KDE4_KPARTS_LIBS} ${X11_Xt_LIB} )

install(TARGETS nspluginviewer DESTINATION ${BIN_INSTALL_DIR})


