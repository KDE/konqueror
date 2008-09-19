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

set(webkitkde_LIB_SRCS
    webkitpart.cpp
    webview.cpp
    webpage.cpp
)

kde4_add_library(webkitkde SHARED ${webkitkde_LIB_SRCS})

target_link_libraries(webkitkde ${KDEWEBKIT_LIBRARIES} ${KDE4_KPARTS_LIBS} ${QT_QTWEBKIT_LIBRARY})

set_target_properties(webkitkde PROPERTIES VERSION 1.0.0 SOVERSION 1)

install(TARGETS webkitkde ${INSTALL_TARGETS_DEFAULT_ARGS})


kde4_add_plugin(webkitkdepart webkitpartfactory.cpp)

target_link_libraries(webkitkdepart webkitkde ${KDEWEBKIT_LIBRARIES} ${KDE4_KPARTS_LIBS} ${QT_QTWEBKIT_LIBRARY})

install(FILES
    webkitkde_export.h
    webkitpart.h
    webview.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/webkitkde/ COMPONENT Devel)

install(TARGETS webkitkdepart DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES webkitpart.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES webkitpart.rc DESTINATION  ${DATA_INSTALL_DIR}/webkitpart)
