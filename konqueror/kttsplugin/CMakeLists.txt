


########### next target ###############

include(MacroOptionalDependPackage)

MACRO_OPTIONAL_DEPEND_PACKAGE(KdeWebKit "kdewebkit")
if(DEPEND_PACKAGE_KdeWebKit)
        macro_optional_find_package(KdeWebKit QUIET)
else(DEPEND_PACKAGE_KdeWebKit)
        set(KDEWEBKIT_FOUND FALSE)
endif(DEPEND_PACKAGE_KdeWebKit)

MACRO_OPTIONAL_DEPEND_PACKAGE(WebKitPart "webkitpart")
if(DEPEND_PACKAGE_WebKitPart)
        macro_optional_find_package(WebKitPart QUIET)
else(DEPEND_PACKAGE_WebKitPart)
        set(WEBKITPART_FOUND FALSE)
endif(DEPEND_PACKAGE_WebKitPart)

if( KDEWEBKIT_FOUND AND WEBKITPART_FOUND )
    set( HAVE_WEBKITKDE true)
    include_directories(${KDEWEBKIT_INCLUDE_DIR})
    include_directories(${WEBKITPART_INCLUDE_DIR})
endif( KDEWEBKIT_FOUND AND WEBKITPART_FOUND )


configure_file(config-kttsplugin.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kttsplugin.h )


set(khtmlkttsdplugin_PART_SRCS khtmlkttsd.cpp )

qt4_add_dbus_interfaces(khtmlkttsdplugin_PART_SRCS ${KDE4_DBUS_INTERFACES_DIR}/org.kde.KSpeech.xml)

kde4_add_plugin(khtmlkttsdplugin ${khtmlkttsdplugin_PART_SRCS})

target_link_libraries(khtmlkttsdplugin  ${KDE4_KHTML_LIBS} )

if( KDEWEBKIT_FOUND AND WEBKITPART_FOUND)
    target_link_libraries(khtmlkttsdplugin ${WEBKITPART_LIBRARIES} ${KDEWEBKIT_LIBRARIES} ${QT_QTWEBKIT_LIBRARY} )
endif(KDEWEBKIT_FOUND AND WEBKITPART_FOUND)


install(TARGETS khtmlkttsdplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
if( KDEWEBKIT_FOUND AND WEBKITPART_FOUND)
   install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${DATA_INSTALL_DIR}/webkitpart/kpartplugins )
endif( KDEWEBKIT_FOUND AND WEBKITPART_FOUND)
