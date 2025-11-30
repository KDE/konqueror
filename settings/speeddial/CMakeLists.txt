
add_definitions(-DTRANSLATION_DOMAIN=\"speeddial\")

########### next target ###############

set(kcm_speeddial_PART_SRCS speeddialconfig.cpp editspeeddialentrydlg.cpp)

ki18n_wrap_ui(kcm_speeddial_PART_SRCS speeddialconfig.ui editspeeddialentrydlg.ui)

kcoreaddons_add_plugin(kcm_speeddial SOURCES ${kcm_speeddial_PART_SRCS} INSTALL_NAMESPACE konqueror_kcms)
target_link_libraries(kcm_speeddial
  KF${KF_MAJOR_VERSION}::KCMUtils
  KF${KF_MAJOR_VERSION}::I18n
  KF${KF_MAJOR_VERSION}::KIOWidgets
  KF${KF_MAJOR_VERSION}::IconWidgets
  KF${KF_MAJOR_VERSION}::KonqSettings
  KF${KF_MAJOR_VERSION}::Konq
)

kcmutils_generate_desktop_file(kcm_speeddial)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kcm_speeddial.desktop DESTINATION ${KDE_INSTALL_APPDIR} RENAME speeddial.desktop)
