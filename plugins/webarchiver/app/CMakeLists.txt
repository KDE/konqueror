add_definitions(-DTRANSLATION_DOMAIN=\"webarchiver\")
find_package(KF5 REQUIRED COMPONENTS I18n Crash KIO Archive XmlGui TextWidgets)

########### next target ###############

set(kcreatewebarchive_SRCS main.cpp archivedialog.cpp)
kconfig_add_kcfg_files(kcreatewebarchive_SRCS settings.kcfgc)

add_executable(kcreatewebarchive ${kcreatewebarchive_SRCS})
target_include_directories(kcreatewebarchive PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/.)
target_link_libraries(kcreatewebarchive
  webarchiverdebug
  KF5::I18n
  KF5::XmlGui
  KF5::TextWidgets
  KF5::Crash
  KF5::KIOCore KF5::KIOWidgets KF5::KIOFileWidgets
  KF5::Archive
  Qt5::Core Qt5::Widgets)

########### install files ###############

install(TARGETS kcreatewebarchive ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES settings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR} RENAME kcreatewebarchive.kcfg)
