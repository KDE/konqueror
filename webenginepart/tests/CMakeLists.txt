add_executable(webenginepart_tester webenginepart_tester.cpp)
target_link_libraries(webenginepart_tester
  kwebenginepartlib
  Qt${KF_MAJOR_VERSION}::Core
  Qt${KF_MAJOR_VERSION}::Gui
  Qt${KF_MAJOR_VERSION}::Widgets
  Qt${KF_MAJOR_VERSION}::WebEngineWidgets
  KF${KF_MAJOR_VERSION}::I18n)
