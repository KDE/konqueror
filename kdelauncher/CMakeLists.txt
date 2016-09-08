project(kdelauncher)

# search packages used by KDE
find_package(ECM REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core DBus Gui Widgets WebEngineWidgets UiTools)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED Archive Bookmarks CoreAddons Config ConfigWidgets DBusAddons KIO KDELibs4Support Parts Activities I18n)
find_package(KF5Parts)
find_package(KF5Sonnet)
include(MacroLibrary)
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

link_directories(${CMAKE_BINARY_DIR}/../src)
include_directories(${CMAKE_SOURCE_DIR}/../src)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(kdelauncher main.cpp )
target_link_libraries(kdelauncher webenginepart Qt5::Core Qt5::Gui Qt5::Widgets Qt5::WebEngineWidgets KF5::I18n KF5::KDELibs4Support)

set_target_properties(kdelauncher PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
