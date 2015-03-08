add_definitions(-DTRANSLATION_DOMAIN=\"kcmperformance\")


########### next target ###############

set(kcm_performance_PART_SRCS kcmperformance.cpp konqueror.cpp system.cpp )


ki18n_wrap_ui(kcm_performance_PART_SRCS konqueror_ui.ui system_ui.ui )

add_library(kcm_performance MODULE ${kcm_performance_PART_SRCS})


target_link_libraries(kcm_performance  KF5::ConfigWidgets KF5::KIOWidgets KF5::I18n Qt5::DBus )

install(TARGETS kcm_performance  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES kcmkonqyperformance.desktop kcmperformance.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )

