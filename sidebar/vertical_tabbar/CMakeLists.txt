########### konqsidebar_verticaltabbar module ###############

set(konqsidebar_verticaltabbar_SRCS
  vertical_tabbar.cpp
  verticaltabbarmodel.cpp
)

add_library(konqsidebar_verticaltabbar MODULE ${konqsidebar_verticaltabbar_SRCS})

generate_export_header(konqsidebar_verticaltabbar BASE_NAME vertical_tabbar EXPORT_FILE_NAME vertical_tabbar_export.h)
target_include_directories(konqsidebar_verticaltabbar PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(konqsidebar_verticaltabbar KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::Parts KF${KF_MAJOR_VERSION}::Konq konqsidebarplugin )

install(TARGETS konqsidebar_verticaltabbar DESTINATION ${KONQ_SIDEBAR_PLUGIN_DIR})
