include_directories(${KDE4_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR})
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

set(kwebkit_LIB_SRCS
    kwebkitpart.cpp
    kwebkitpart_ext.cpp
    kwebkitpartfactory.cpp
    webview.cpp
    webpage.cpp
    websslinfo.cpp
    webhistoryinterface.cpp
    networkaccessmanager.cpp
    webpluginfactory.cpp
    settings/webkitsettings.cpp
    settings/webkit_filter.cpp
    ui/searchbar.cpp
    ui/passwordbar.cpp
)

kde4_add_ui_files(kwebkit_LIB_SRCS
                  ui/searchbar.ui
)

########################### kwebkitpart #############################
kde4_add_plugin(kwebkitpart ${kwebkit_LIB_SRCS})

target_link_libraries(kwebkitpart ${KDE4_KPARTS_LIBS} ${KDE4_KDEWEBKIT_LIBS} ${QT_QTWEBKIT_LIBRARY} ${QT_QTUITOOLS_LIBRARY})
########################### kwebkitpart #############################

install(TARGETS kwebkitpart DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kwebkitpart.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES kwebkitpart.rc DESTINATION ${DATA_INSTALL_DIR}/kwebkitpart)

install(FILES error.html DESTINATION ${DATA_INSTALL_DIR}/kwebkitpart)
