find_package(Qt5 REQUIRED COMPONENTS Core)

########### next target ###############

set(webarchiverplugin_DEBUG_SRCS "")
ecm_qt_declare_logging_category(webarchiverplugin_DEBUG_SRCS HEADER webarchiverdebug.h IDENTIFIER WEBARCHIVERPLUGIN_LOG CATEGORY_NAME org.kde.konqueror.webarchiver)
add_library(webarchiverdebug STATIC ${webarchiverplugin_DEBUG_SRCS})
target_link_libraries(webarchiverdebug Qt5::Core)
