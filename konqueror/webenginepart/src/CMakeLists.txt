include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR})

set(kwebenginepartlib_LIB_SRCS
    webenginepart.cpp
    webenginepart_ext.cpp
    webview.cpp
    webpage.cpp
    websslinfo.cpp
    webhistoryinterface.cpp
    settings/webenginesettings.cpp
    settings/webengine_filter.cpp
    ui/searchbar.cpp
    ui/passwordbar.cpp
    ui/featurepermissionbar.cpp
)

qt5_wrap_ui(kwebenginepartlib_LIB_SRCS
                  ui/searchbar.ui
)

add_library(kwebenginepartlib ${kwebenginepartlib_LIB_SRCS})

generate_export_header(kwebenginepartlib)

target_link_libraries(kwebenginepartlib Qt5::Core Qt5::DBus Qt5::Gui Qt5::Widgets Qt5::WebEngineWidgets Qt5::PrintSupport KF5::Parts KF5::SonnetCore)

target_include_directories(kwebenginepartlib PUBLIC
   "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>"
)

install(TARGETS kwebenginepartlib ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

add_library(webenginepart MODULE webenginepartfactory.cpp)

target_link_libraries(webenginepart kwebenginepartlib)

install(TARGETS webenginepart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/parts)

install(FILES webenginepart.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES webenginepart.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/webenginepart)
