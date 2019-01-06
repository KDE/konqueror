include_directories (${CMAKE_CURRENT_SOURCE_DIR})
find_package(KF5 REQUIRED COMPONENTS JobWidgets)

# TODO: rewrite to KDirModel
if (${QT_QT3SUPPORT_FOUND})
add_subdirectory( trees )
endif()

add_subdirectory( web_module )
add_subdirectory( history_module )
add_subdirectory( places_module )
add_subdirectory( default_entries )
if(BUILD_TESTING)
    add_subdirectory( test )
endif()

########### konqsidebarplugin lib: contains the base class for plugins ###############

set(konqsidebarplugin_LIB_SRCS konqsidebarplugin.cpp )

add_library(konqsidebarplugin SHARED ${konqsidebarplugin_LIB_SRCS})

target_link_libraries(konqsidebarplugin KF5::Parts KF5::KDELibs4Support KF5::KIOFileWidgets)
set_target_properties(konqsidebarplugin PROPERTIES VERSION ${KONQUEROR_LIB_VERSION} SOVERSION "5" )
install(TARGETS konqsidebarplugin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

########### konq_sidebar plugin: contains the KParts used by konqueror ###############

set(konq_sidebar_PART_SRCS 
 sidebar_part.cpp
 sidebar_widget.cpp
 module_manager.cpp
 konqmultitabbar.cpp
)

add_library(konq_sidebar MODULE ${konq_sidebar_PART_SRCS})

target_link_libraries(konq_sidebar KF5::Parts KF5::Konq konqsidebarplugin KF5::KCMUtils KF5::KDELibs4Support )

install(TARGETS konq_sidebar DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### install files ###############

install( FILES konq_sidebartng.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES konqsidebartngrc  DESTINATION  ${KDE_INSTALL_CONFDIR} )
install( FILES konqsidebarplugin.h DESTINATION ${KDE_INSTALL_INCLUDEDIR} COMPONENT Devel )
