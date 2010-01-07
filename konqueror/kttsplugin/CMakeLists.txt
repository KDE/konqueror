


########### next target ###############

include(MacroOptionalDependPackage)

MACRO_OPTIONAL_DEPEND_PACKAGE(WebKitPart "kwebkitpart")
if(DEPEND_PACKAGE_WebKitPart)
        macro_optional_find_package(WebKitPart QUIET)
else(DEPEND_PACKAGE_WebKitPart)
        set(WEBKITPART_FOUND FALSE)
endif(DEPEND_PACKAGE_WebKitPart)

if(WEBKITPART_FOUND)
    set(HAVE_WEBKITPART true)
    include_directories(${WEBKITPART_INCLUDE_DIR})
endif(WEBKITPART_FOUND)


configure_file(config-kttsplugin.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kttsplugin.h )


set(khtmlkttsdplugin_PART_SRCS khtmlkttsd.cpp )

qt4_add_dbus_interfaces(khtmlkttsdplugin_PART_SRCS ${KDE4_DBUS_INTERFACES_DIR}/org.kde.KSpeech.xml)

kde4_add_plugin(khtmlkttsdplugin ${khtmlkttsdplugin_PART_SRCS})

target_link_libraries(khtmlkttsdplugin  ${KDE4_KHTML_LIBS} )

if(WEBKITPART_FOUND)
    target_link_libraries(khtmlkttsdplugin ${WEBKITPART_LIBRARIES} ${KDE4_KDEWEBKIT_LIBS} ${QT_QTWEBKIT_LIBRARY} )
endif(WEBKITPART_FOUND)


install(TARGETS khtmlkttsdplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
if(WEBKITPART_FOUND)
   install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart/kpartplugins )
endif(WEBKITPART_FOUND)
