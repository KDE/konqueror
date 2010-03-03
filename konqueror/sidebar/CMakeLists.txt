include_directories (${CMAKE_CURRENT_SOURCE_DIR})

# TODO: rewrite to KDirModel
if (${QT_QT3SUPPORT_FOUND})
add_subdirectory( trees )
endif()

add_subdirectory( web_module )
add_subdirectory( history_module )
add_subdirectory( default_entries )
add_subdirectory( test )

########### konqsidebarplugin lib: contains the base class for plugins ###############

set(konqsidebarplugin_LIB_SRCS konqsidebarplugin.cpp )

kde4_add_library(konqsidebarplugin SHARED ${konqsidebarplugin_LIB_SRCS})

target_link_libraries(konqsidebarplugin ${KDE4_KPARTS_LIBS} )

set_target_properties(konqsidebarplugin PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS konqsidebarplugin ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### konq_sidebar plugin: contains the KParts used by konqueror ###############

set(konq_sidebar_PART_SRCS 
 sidebar_part.cpp
 sidebar_widget.cpp
 module_manager.cpp
 konqmultitabbar.cpp
)

kde4_add_plugin(konq_sidebar ${konq_sidebar_PART_SRCS})

target_link_libraries(konq_sidebar ${KDE4_KPARTS_LIBS} konq konqsidebarplugin ${KDE4_KUTILS_LIBS} )

install(TARGETS konq_sidebar DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES konq_sidebartng.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES konqsidebartngrc  DESTINATION  ${CONFIG_INSTALL_DIR} )
install( FILES konqsidebarplugin.h DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel )
