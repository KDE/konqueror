if(KDE4_BUILD_TESTS)
include_directories( ${CMAKE_SOURCE_DIR}/libltdl ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../ )

########### next target ###############

set(testnsplugin_SRCS testnsplugin.cpp ${nsplugins_SRCS} )

qt4_add_dbus_adaptor( testnsplugin_SRCS ${CMAKE_SOURCE_DIR}/nsplugins/org.kde.nsplugins.CallBack.xml ${CMAKE_SOURCE_DIR}/nsplugins/plugin_part.h PluginPart
)

set(nsplugins_class_xml  ${CMAKE_SOURCE_DIR}/nsplugins/viewer/org.kde.nsplugins.class.xml)
QT4_ADD_DBUS_INTERFACE( testnsplugin_SRCS ${nsplugins_class_xml} nsplugins_class_interface )

set(nsplugins_instance_xml  ${CMAKE_SOURCE_DIR}/nsplugins/viewer/org.kde.nsplugins.instance.xml)
QT4_ADD_DBUS_INTERFACE( testnsplugin_SRCS ${nsplugins_instance_xml} nsplugins_instance_interface )

set(nsplugins_viewer_xml  ${CMAKE_SOURCE_DIR}/nsplugins/viewer/org.kde.nsplugins.viewer.xml)
QT4_ADD_DBUS_INTERFACE( testnsplugin_SRCS ${nsplugins_viewer_xml} nsplugins_viewer_interface )


kde4_automoc(${testnsplugin_SRCS})

kde4_add_executable(testnsplugin ${testnsplugin_SRCS})

target_link_libraries(testnsplugin  ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS} ${QT_QT3SUPPORT_LIBRARY} ${QT_QTGUI_LIBRARY}  )

endif(KDE4_BUILD_TESTS)

