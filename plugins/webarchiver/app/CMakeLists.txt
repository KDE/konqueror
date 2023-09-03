add_definitions(-DTRANSLATION_DOMAIN=\"webarchiver\")
find_package(KF${KF_MAJOR_VERSION} REQUIRED COMPONENTS I18n Crash KIO Archive XmlGui TextWidgets)

########### next target ###############

set(kcreatewebarchive_SRCS main.cpp archivedialog.cpp)
kconfig_add_kcfg_files(kcreatewebarchive_SRCS settings.kcfgc)

add_executable(kcreatewebarchive ${kcreatewebarchive_SRCS})
target_include_directories(kcreatewebarchive PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/.)
target_link_libraries(kcreatewebarchive
  webarchiverdebug
  KF${KF_MAJOR_VERSION}::I18n
  KF${KF_MAJOR_VERSION}::XmlGui
  KF${KF_MAJOR_VERSION}::TextWidgets
  KF${KF_MAJOR_VERSION}::Crash
  KF${KF_MAJOR_VERSION}::KIOCore
  KF${KF_MAJOR_VERSION}::KIOWidgets
  KF${KF_MAJOR_VERSION}::KIOFileWidgets
  KF${KF_MAJOR_VERSION}::Archive
  Qt${KF_MAJOR_VERSION}::Core
  Qt${KF_MAJOR_VERSION}::Widgets)

########### install files ###############

install(TARGETS kcreatewebarchive ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES settings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR} RENAME kcreatewebarchive.kcfg)
