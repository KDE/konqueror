include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR})

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
    ui/featurepermissionbar.cpp
)

qt5_wrap_ui(kwebkit_LIB_SRCS
                  ui/searchbar.ui
)

########################### kwebkitpart #############################
add_library(kwebkitpart ${kwebkit_LIB_SRCS})

target_link_libraries(kwebkitpart Qt5::Core Qt5::DBus Qt5::Gui Qt5::Widgets Qt5::WebKit Qt5::WebKitWidgets KF5::WebKit KF5::Parts KF5::KDELibs4Support KF5::SonnetCore)
########################### kwebkitpart #############################

install(TARGETS kwebkitpart DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kwebkitpart.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES kwebkitpart.rc DESTINATION ${DATA_INSTALL_DIR}/kwebkitpart)

install(FILES error.html DESTINATION ${DATA_INSTALL_DIR}/kwebkitpart)
