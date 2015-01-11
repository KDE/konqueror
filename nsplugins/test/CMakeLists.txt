set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../ )

########### next target ###############

set(testnsplugin_SRCS testnsplugin.cpp ${nsplugins_SRCS} )

qt5_add_dbus_adaptor( testnsplugin_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../org.kde.nsplugins.CallBack.xml ${CMAKE_CURRENT_SOURCE_DIR}/../plugin_part.h PluginPart
)

set(nsplugins_class_xml  ${CMAKE_CURRENT_SOURCE_DIR}/../viewer/org.kde.nsplugins.class.xml)
qt5_add_dbus_interface( testnsplugin_SRCS ${nsplugins_class_xml} nsplugins_class_interface )

set(nsplugins_instance_xml  ${CMAKE_CURRENT_SOURCE_DIR}/../viewer/org.kde.nsplugins.instance.xml)
set_source_files_properties(${nsplugins_instance_xml} PROPERTIES INCLUDE "comm/dbustypes.h")
qt5_add_dbus_interface( testnsplugin_SRCS ${nsplugins_instance_xml} nsplugins_instance_interface )

set(nsplugins_viewer_xml  ${CMAKE_CURRENT_SOURCE_DIR}/../viewer/org.kde.nsplugins.viewer.xml)
qt5_add_dbus_interface( testnsplugin_SRCS ${nsplugins_viewer_xml} nsplugins_viewer_interface )

add_executable(testnsplugin ${testnsplugin_SRCS})
ecm_mark_as_test(testnsplugin)

target_link_libraries(testnsplugin KF5::Parts Qt5::DBus)
