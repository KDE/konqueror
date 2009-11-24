project(webkitpart)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

find_package(KdeWebKit REQUIRED)

include_directories(${KDE4_INCLUDES} ${KDEWEBKIT_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

add_subdirectory(cmake)
add_subdirectory(includes)

set(kwebkit_LIB_SRCS
    webkitpart.cpp
    webkitpart_ext.cpp
    webview.cpp
    webpage.cpp
    websslinfo.cpp
    ui/searchbar.cpp
    settings/webkitsettings.cpp
    settings/khtml_filter.cpp
)

kde4_add_ui_files(kwebkit_LIB_SRCS
    ui/searchbar.ui
)

kde4_add_library(kwebkit SHARED ${kwebkit_LIB_SRCS})

target_link_libraries(kwebkit ${KDEWEBKIT_LIBRARIES} ${KDE4_KPARTS_LIBS} ${QT_QTWEBKIT_LIBRARY} ${QT_QTUITOOLS_LIBRARY})

set_target_properties(kwebkit PROPERTIES VERSION 1.0.0 SOVERSION 1)

install(TARGETS kwebkit ${INSTALL_TARGETS_DEFAULT_ARGS})


kde4_add_plugin(kwebkitpart webkitpartfactory.cpp)

target_link_libraries(kwebkitpart kwebkit ${KDEWEBKIT_LIBRARIES} ${KDE4_KPARTS_LIBS} ${QT_QTWEBKIT_LIBRARY})

install(FILES
        webkitkde_export.h
        webkitpart.h
        DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel)

install(TARGETS kwebkitpart DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES webkitpart.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES webkitpart.rc DESTINATION  ${DATA_INSTALL_DIR}/webkitpart)

install(FILES error.html DESTINATION ${DATA_INSTALL_DIR}/webkitpart)
