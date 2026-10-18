add_definitions(-DTRANSLATION_DOMAIN=\"permissions_kcm\")

set(kcm_permissions_PART_SRCS permissions.cpp)

ki18n_wrap_ui(kcm_permissions_PART_SRCS permissions.ui)

kcoreaddons_add_plugin(kcm_permissions SOURCES ${kcm_permissions_PART_SRCS} INSTALL_NAMESPACE konqueror_kcms)
target_link_libraries(kcm_permissions
  KF${KF_MAJOR_VERSION}::ConfigWidgets
  KF${KF_MAJOR_VERSION}::KCMUtils
  KF${KF_MAJOR_VERSION}::I18n
  KF${KF_MAJOR_VERSION}::KIOWidgets
  KF${KF_MAJOR_VERSION}::IconWidgets
  KF${KF_MAJOR_VERSION}::ItemViews
  KF${KF_MAJOR_VERSION}::KonqSettings
  Qt${KF_MAJOR_VERSION}::WebEngineWidgets
  kwebenginepartlib
)

kcmutils_generate_desktop_file(kcm_permissions)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kcm_permissions.desktop DESTINATION ${KDE_INSTALL_APPDIR} RENAME permissions.desktop)

