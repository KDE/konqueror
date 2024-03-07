
option(THUMBNAIL_USE_WEBKIT "Use WebKit to generate HTML and web archive thumbnails" OFF)

if (THUMBNAIL_USE_WEBKIT)
  find_package(Qt${KF_MAJOR_VERSION} REQUIRED WebKit WebKitWidgets)
  add_definitions(-DTHUMBNAIL_USE_WEBKIT)
else ()
  find_package(Qt${KF_MAJOR_VERSION} REQUIRED COMPONENTS WebEngineCore WebEngineWidgets)
endif ()

find_package(KF${KF_MAJOR_VERSION} REQUIRED COMPONENTS KIO Archive)

########### next target ###############

set(webarchivethumbnail_SRCS webarchivecreator.cpp )
add_library(webarchivethumbnail MODULE ${webarchivethumbnail_SRCS} ${webarchiverplugin_DEBUG_SRCS})
target_include_directories(webarchivethumbnail PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..)
target_link_libraries(webarchivethumbnail
  KF${KF_MAJOR_VERSION}::KIOWidgets
  KF${KF_MAJOR_VERSION}::Archive
)

if (THUMBNAIL_USE_WEBKIT)
  target_link_libraries(webarchivethumbnail Qt${KF_MAJOR_VERSION}::WebKit Qt${KF_MAJOR_VERSION}::WebKitWidgets)
else ()
  target_link_libraries(webarchivethumbnail Qt${KF_MAJOR_VERSION}::WebEngineCore Qt${KF_MAJOR_VERSION}::WebEngineWidgets)
endif ()

########### install files ###############

install(TARGETS webarchivethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${KF_MAJOR_VERSION}/thumbcreator)
