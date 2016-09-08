include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR})

set(webenginepart_LIB_SRCS
    webenginepart.cpp
    webenginepart_ext.cpp
    webenginepartfactory.cpp
    webview.cpp
    webpage.cpp
    websslinfo.cpp
    webhistoryinterface.cpp
    settings/webenginesettings.cpp
    settings/webengine_filter.cpp
    ui/searchbar.cpp
    ui/passwordbar.cpp
    ui/featurepermissionbar.cpp
    webenginepart.desktop
)

qt5_wrap_ui(webenginepart_LIB_SRCS
                  ui/searchbar.ui
)

########################### kwebkitpart #############################
add_library(webenginepart ${webenginepart_LIB_SRCS})

target_link_libraries(webenginepart Qt5::Core Qt5::DBus Qt5::Gui Qt5::Widgets Qt5::WebEngineWidgets KF5::Parts KF5::KDELibs4Support KF5::SonnetCore)
########################### kwebkitpart #############################

install(TARGETS webenginepart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/parts)

install(FILES webenginepart.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES webenginepart.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/webenginepart)
