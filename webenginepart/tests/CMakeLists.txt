add_executable(webenginepart_tester webenginepart_tester.cpp)
target_link_libraries(webenginepart_tester kwebenginepartlib Qt5::Core Qt5::Gui Qt5::Widgets Qt5::WebEngineWidgets KF5::I18n)
