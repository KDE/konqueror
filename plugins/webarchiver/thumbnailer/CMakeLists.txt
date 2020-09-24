
option(THUMBNAIL_USE_WEBKIT "Use WebKit to generate HTML and web archive thumbnails" OFF)

if (THUMBNAIL_USE_WEBKIT)
  find_package(Qt5 REQUIRED WebKit WebKitWidgets)
  add_definitions(-DTHUMBNAIL_USE_WEBKIT)
else ()
  find_package(Qt5 REQUIRED COMPONENTS WebEngine WebEngineWidgets)
endif ()

find_package(KF5 REQUIRED COMPONENTS KIO Archive)

########### next target ###############

set(webarchivethumbnail_SRCS webarchivecreator.cpp )
add_library(webarchivethumbnail MODULE ${webarchivethumbnail_SRCS})
target_include_directories(webarchivethumbnail PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/.)
target_link_libraries(webarchivethumbnail
  webarchiverdebug
  KF5::KIOWidgets
  KF5::Archive
)

if (THUMBNAIL_USE_WEBKIT)
  target_link_libraries(webarchivethumbnail Qt5::WebKit Qt5::WebKitWidgets)
else ()
  target_link_libraries(webarchivethumbnail Qt5::WebEngine Qt5::WebEngineWidgets)
endif ()

########### install files ###############

install(TARGETS webarchivethumbnail  DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install(FILES webarchivethumbnail.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
