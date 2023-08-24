include_directories (${CMAKE_CURRENT_SOURCE_DIR})
find_package(KF${KF_MAJOR_VERSION} REQUIRED COMPONENTS JobWidgets IconThemes)

#Used in subdirectories to install the plugins in the correct place
#NOTE: it must be set before calling add_subdirectory
set(KONQ_SIDEBAR_PLUGIN_DIR "${KDE_INSTALL_PLUGINDIR}/konqueror/sidebar")

#TODO KF6: check what to do with this: either port it to Qt6 or remove it
if (${QT_QT3SUPPORT_FOUND})
add_subdirectory( trees )
endif()

add_subdirectory( bookmarks_module )
add_subdirectory( tree_module )
#TODO KF6: check what to do with this: either port it to WebEnginePart or remove it
# add_subdirectory( web_module ) # requires refactoring away from KHtml
add_subdirectory( history_module )
add_subdirectory( places_module )
add_subdirectory( default_entries )
if(BUILD_TESTING)
    add_subdirectory( test )
endif()

ecm_qt_declare_logging_category(sidebar_DEBUG_SRCS HEADER sidebar_debug.h IDENTIFIER SIDEBAR_LOG CATEGORY_NAME org.kde.konqueror.sidebar)

########### konqsidebarplugin lib: contains the base class for plugins ###############

set(konqsidebarplugin_LIB_SRCS konqsidebarplugin.cpp )

add_library(konqsidebarplugin SHARED ${konqsidebarplugin_LIB_SRCS} ${sidebar_DEBUG_SRCS})

target_link_libraries(konqsidebarplugin KF${KF_MAJOR_VERSION}::Parts KF${KF_MAJOR_VERSION}::KIOFileWidgets)
set_target_properties(konqsidebarplugin PROPERTIES VERSION ${KONQUEROR_LIB_VERSION} SOVERSION "${KF_MAJOR_VERSION}" )
install(TARGETS konqsidebarplugin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

########### konq_sidebar plugin: contains the KParts used by konqueror ###############

set(konq_sidebar_VERSION 0.2)

configure_file(konq_sidebartng.json.in ${CMAKE_CURRENT_BINARY_DIR}/konq_sidebartng.json)

set(konq_sidebar_PART_SRCS 
 sidebar_part.cpp
 sidebar_widget.cpp
 module_manager.cpp
 konqmultitabbar.cpp
)

add_library(konq_sidebar MODULE ${konq_sidebar_PART_SRCS} ${sidebar_DEBUG_SRCS})

target_link_libraries(konq_sidebar
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::Parts
    KF${KF_MAJOR_VERSION}::Konq
    KF${KF_MAJOR_VERSION}::KCMUtils
    KF${KF_MAJOR_VERSION}::IconThemes
    KF${KF_MAJOR_VERSION}::KIOFileWidgets
    konqsidebarplugin)

if (KF_MAJOR_VERSION STRGREATER "5")
    target_link_libraries(konq_sidebar KF${KF_MAJOR_VERSION}::IconWidgets)
endif()

install(TARGETS konq_sidebar DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${KF_MAJOR_VERSION}/parts)

########### install files ###############

install( FILES konqsidebartngrc  DESTINATION  ${KDE_INSTALL_CONFDIR} )
install( FILES konqsidebarplugin.h DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF} COMPONENT Devel )
