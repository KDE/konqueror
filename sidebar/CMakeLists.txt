include_directories (${CMAKE_CURRENT_SOURCE_DIR})
find_package(KF5 REQUIRED COMPONENTS JobWidgets IconThemes)

if (${QT_QT3SUPPORT_FOUND})
add_subdirectory( trees )
endif()

add_subdirectory( bookmarks_module )
add_subdirectory( tree_module )
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

target_link_libraries(konqsidebarplugin KF5::Parts KF5::KIOFileWidgets)
set_target_properties(konqsidebarplugin PROPERTIES VERSION ${KONQUEROR_LIB_VERSION} SOVERSION "5" )
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

target_link_libraries(konq_sidebar KF5::I18n KF5::Parts KF5::Konq konqsidebarplugin KF5::KCMUtils KF5::IconThemes)

install(TARGETS konq_sidebar DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/parts)

########### install files ###############

install( FILES konqsidebartngrc  DESTINATION  ${KDE_INSTALL_CONFDIR} )
install( FILES konqsidebarplugin.h DESTINATION ${KDE_INSTALL_INCLUDEDIR} COMPONENT Devel )
