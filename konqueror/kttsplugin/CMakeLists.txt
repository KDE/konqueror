


########### next target ###############

include(MacroOptionalDependPackage)

MACRO_OPTIONAL_DEPEND_PACKAGE(WebKitKde "webkitkde")
if(DEPEND_PACKAGE_WebKitKde)
        macro_optional_find_package(WebKitKde QUIET)
else(DEPEND_PACKAGE_WebKitKde)
        set(WEBKITKDE_FOUND FALSE)
endif(DEPEND_PACKAGE_WebKitKde)

macro_bool_to_01(WEBKITKDE_FOUND HAVE_WEBKITKDE)

if( WEBKITKDE_FOUND )
   include_directories(${WEBKITKDE_INCLUDE_DIR})
   FIND_PATH(WEBVIEW_PATH webkitkde/webview.h PATH ${WEBKITKDE_INCLUDE_DIR})
   macro_bool_to_01(WEBVIEW_PATH HAVE_WEBVIEW )
endif(WEBKITKDE_FOUND)


configure_file(config-kttsplugin.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kttsplugin.h )


set(khtmlkttsdplugin_PART_SRCS khtmlkttsd.cpp )

qt4_add_dbus_interfaces(khtmlkttsdplugin_PART_SRCS ${KDE4_DBUS_INTERFACES_DIR}/org.kde.KSpeech.xml)

kde4_add_plugin(khtmlkttsdplugin ${khtmlkttsdplugin_PART_SRCS})

target_link_libraries(khtmlkttsdplugin  ${KDE4_KHTML_LIBS} )

if( WEBKITKDE_FOUND)
    target_link_libraries(khtmlkttsdplugin ${WEBKITKDE_LIBRARIES} ${QT_QTWEBKIT_LIBRARY} )
endif(WEBKITKDE_FOUND)


install(TARGETS khtmlkttsdplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
if( WEBKITKDE_FOUND)
   install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${DATA_INSTALL_DIR}/webkitpart/kpartplugins )
endif( WEBKITKDE_FOUND)
