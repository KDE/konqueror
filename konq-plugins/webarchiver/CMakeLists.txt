find_package(KF5 REQUIRED COMPONENTS KIO Archive KHtml)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${KDE4_ENABLE_EXCEPTIONS}")
########### next target ###############

set(webarchiverplugin_PART_SRCS plugin_webarchiver.cpp archivedialog.cpp )

ki18n_wrap_ui(webarchiverplugin_PART_SRCS archiveviewbase.ui )

add_library(webarchiverplugin MODULE ${webarchiverplugin_PART_SRCS})

target_link_libraries(webarchiverplugin KF5::KHtml KF5::KDELibs4Support KF5::KIOCore KF5::Archive)
install(TARGETS webarchiverplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(webarchivethumbnail_PART_SRCS webarchivecreator.cpp )

add_library(webarchivethumbnail MODULE ${webarchivethumbnail_PART_SRCS})

target_link_libraries(webarchivethumbnail KF5::KHtml KF5::KDELibs4Support KF5::KIOCore)

install(TARGETS webarchivethumbnail  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_webarchiver.rc plugin_webarchiver.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES webarchivethumbnail.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

ecm_install_icons( ${ICON_INSTALL_DIR}   )

