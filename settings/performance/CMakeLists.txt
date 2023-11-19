add_definitions(-DTRANSLATION_DOMAIN=\"kcmperformance\")


########### next target ###############

set(kcm_performance_PART_SRCS kcmperformance.cpp konqueror.cpp system.cpp )

ki18n_wrap_ui(kcm_performance_PART_SRCS konqueror_ui.ui system_ui.ui )

kcoreaddons_add_plugin(kcm_performance SOURCES ${kcm_performance_PART_SRCS} INSTALL_NAMESPACE "konqueror_kcms")

target_link_libraries(kcm_performance
  KF${KF_MAJOR_VERSION}::ConfigWidgets
  KF${KF_MAJOR_VERSION}::KCMUtils
  KF${KF_MAJOR_VERSION}::KIOWidgets
  KF${KF_MAJOR_VERSION}::I18n
  Qt${KF_MAJOR_VERSION}::DBus)
