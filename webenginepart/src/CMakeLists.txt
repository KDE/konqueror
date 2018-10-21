find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Wallet)

add_definitions(-DTRANSLATION_DOMAIN=\"webenginepart\")

if(BUILD_TESTING)
  add_definitions(-DBUILD_TESTING)
endif(BUILD_TESTING)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR})

set(kwebenginepartlib_LIB_SRCS
    webenginepart.cpp
    webenginepart_ext.cpp
    webengineview.cpp
    webenginepage.cpp
    websslinfo.cpp
    webhistoryinterface.cpp
    webenginepartdownloadmanager.cpp
    webenginewallet.cpp
    webengineparterrorschemehandler.cpp
    webengineparthtmlembedder.cpp
    webenginepartkiohandler.cpp
    webenginepartcookiejar.cpp
    settings/webenginesettings.cpp
    settings/webengine_filter.cpp
    ui/searchbar.cpp
    ui/passwordbar.cpp
    ui/featurepermissionbar.cpp
)

qt5_wrap_ui(kwebenginepartlib_LIB_SRCS
                  ui/searchbar.ui
)
ecm_qt_declare_logging_category(kwebenginepartlib_LIB_SRCS
    HEADER webenginepart_debug.h
    IDENTIFIER WEBENGINEPART_LOG
    CATEGORY_NAME org.kde.webenginepart
)
add_library(kwebenginepartlib ${kwebenginepartlib_LIB_SRCS})

generate_export_header(kwebenginepartlib)

target_link_libraries(kwebenginepartlib
    PUBLIC
        Qt5::Core
        Qt5::DBus
        Qt5::Gui
        Qt5::Widgets
        Qt5::WebEngineWidgets
        KF5::Parts
        KF5::Wallet
    PRIVATE
        Qt5::PrintSupport
        KF5::SonnetCore
        KF5::IconThemes #for KIconLoader used by WebEnginePartErrorSchemeHandler
        KF5::WindowSystem  # for KUserTimestamp
)

target_include_directories(kwebenginepartlib PUBLIC
   "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>"
)
set_target_properties(kwebenginepartlib PROPERTIES OUTPUT_NAME kwebenginepart)

install(TARGETS kwebenginepartlib ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


add_library(webenginepart MODULE webenginepartfactory.cpp)
kcoreaddons_desktop_to_json(webenginepart webenginepart.desktop)

target_link_libraries(webenginepart kwebenginepartlib)

install(TARGETS webenginepart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/parts)

install(FILES webenginepart.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES webenginepart.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/webenginepart)

install(FILES error.html DESTINATION ${KDE_INSTALL_DATADIR}/webenginepart)
