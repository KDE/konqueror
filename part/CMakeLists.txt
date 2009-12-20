project(kwebkitpart)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

if(${KDE_VERSION} VERSION_LESS 4.3.74)
    find_package(KdeWebKit REQUIRED)
endif(${KDE_VERSION} VERSION_LESS 4.3.74)

include_directories(${KDE4_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/..)
if(${KDE_VERSION} VERSION_LESS 4.3.74)
    include_directories(${KDEWEBKIT_INCLUDE_DIR})
endif(${KDE_VERSION} VERSION_LESS 4.3.74)
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

add_subdirectory(cmake)
add_subdirectory(includes)

set(kwebkit_LIB_SRCS
    kwebkitpart.cpp
    kwebkitpart_ext.cpp
    webview.cpp
    webpage.cpp
    websslinfo.cpp
    networkaccessmanager.cpp
    settings/webkitsettings.cpp
    settings/khtml_filter.cpp
    ui/searchbar.cpp
    ui/passwordbar.cpp
)

kde4_add_ui_files(kwebkit_LIB_SRCS
                  ui/searchbar.ui
                  ui/passwordbar.ui
)

########################### kwebkit #############################

kde4_add_library(kwebkit SHARED ${kwebkit_LIB_SRCS})

if(${KDE_VERSION} VERSION_LESS 4.3.74)
    target_link_libraries(kwebkit ${KDE4_KPARTS_LIBS} ${KDEWEBKIT_LIBRARIES} ${QT_QTWEBKIT_LIBRARY} ${QT_QTUITOOLS_LIBRARY})
else(${KDE_VERSION} VERSION_LESS 4.3.74)
    target_link_libraries(kwebkit ${KDE4_KPARTS_LIBS} ${KDE4_KDEWEBKIT_LIBS} ${QT_QTWEBKIT_LIBRARY} ${QT_QTUITOOLS_LIBRARY})
endif(${KDE_VERSION} VERSION_LESS 4.3.74)

set_target_properties(kwebkit PROPERTIES VERSION 1.0.0 SOVERSION 1)

########################### kwebkit #############################


########################### kwebkitpart #############################

kde4_add_plugin(kwebkitpart kwebkitpartfactory.cpp)

target_link_libraries(kwebkitpart ${KDE4_KPARTS_LIBS} ${QT_QTWEBKIT_LIBRARY} kwebkit)

set_target_properties(kwebkitpart PROPERTIES VERSION 1.0.0 SOVERSION 1)

########################### kwebkitpart #############################

install(FILES
        kwebkit_export.h
        kwebkitpart.h
        DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel)

install(TARGETS kwebkit ${INSTALL_TARGETS_DEFAULT_ARGS})

install(TARGETS kwebkitpart DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kwebkitpart.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES kwebkitpart.rc DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart)

install(FILES error.html DESTINATION ${DATA_INSTALL_DIR}/kwebkitpart)
