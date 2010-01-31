


########### next target ###############

include(MacroOptionalDependPackage)

MACRO_OPTIONAL_DEPEND_PACKAGE(KWebKitPart "kwebkitpart")
if(DEPEND_PACKAGE_KWebKitPart)
        macro_optional_find_package(KWebKitPart QUIET)
else(DEPEND_PACKAGE_KWebKitPart)
        set(KWEBKITPART_FOUND FALSE)
endif(DEPEND_PACKAGE_KWebKitPart)

if(KWEBKITPART_FOUND)
    set(HAVE_KWEBKITPART true)
    include_directories(${KWEBKITPART_INCLUDE_DIR})
endif(KWEBKITPART_FOUND)


configure_file(config-kttsplugin.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kttsplugin.h )


set(khtmlkttsdplugin_PART_SRCS khtmlkttsd.cpp )

qt4_add_dbus_interfaces(khtmlkttsdplugin_PART_SRCS ${KDE4_DBUS_INTERFACES_DIR}/org.kde.KSpeech.xml)

kde4_add_plugin(khtmlkttsdplugin ${khtmlkttsdplugin_PART_SRCS})

target_link_libraries(khtmlkttsdplugin  ${KDE4_KHTML_LIBS} )

if(KWEBKITPART_FOUND)
    target_link_libraries(khtmlkttsdplugin ${KWEBKITPART_LIBRARIES} ${KDE4_KDEWEBKIT_LIBS} ${QT_QTWEBKIT_LIBRARY} )
endif(KWEBKITPART_FOUND)


install(TARGETS khtmlkttsdplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
if(KWEBKITPART_FOUND)
   install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart/kpartplugins )
endif(KWEBKITPART_FOUND)
