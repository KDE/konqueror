find_package(Qt${KF_MAJOR_VERSION} REQUIRED COMPONENTS WebEngineCore WebEngineWidgets)

find_package(KF${KF_MAJOR_VERSION} REQUIRED COMPONENTS KIO Archive)

########### next target ###############

set(webarchivethumbnail_SRCS webarchivecreator.cpp )
add_library(webarchivethumbnail MODULE ${webarchivethumbnail_SRCS} ${webarchiverplugin_DEBUG_SRCS})
target_include_directories(webarchivethumbnail PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..)
target_link_libraries(webarchivethumbnail
  KF${KF_MAJOR_VERSION}::KIOWidgets
  KF${KF_MAJOR_VERSION}::Archive
  Qt${KF_MAJOR_VERSION}::WebEngineCore
  Qt${KF_MAJOR_VERSION}::WebEngineWidgets
)

########### install files ###############

install(TARGETS webarchivethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${KF_MAJOR_VERSION}/thumbcreator)
