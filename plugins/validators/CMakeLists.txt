add_definitions(-DTRANSLATION_DOMAIN=\"validatorsplugin\")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

find_package(LibTidy)
find_package(KF5 REQUIRED KHtml WidgetsAddons IconThemes)
set_package_properties(LibTidy PROPERTIES DESCRIPTION "HTML Tidy"
                       URL "http://www.html-tidy.org/"
                       TYPE OPTIONAL
                       PURPOSE "Gives the Konqueror validators plugin the ability to validate HTML"
                      )

macro_bool_to_01(LIBTIDY_FOUND HAVE_TIDY)

configure_file(config-konq-validator.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konq-validator.h)


if (LIBTIDY_FOUND)
  include_directories(${LIBTIDY_INCLUDE_DIR})
endif (LIBTIDY_FOUND)

add_subdirectory(pics)

########### next target ###############

set(validatorsplugin_PART_SRCS
  clickiconlabel.cpp
  plugin_validators.cpp
  validatorsdialog.cpp
)
if (LIBTIDY_FOUND)
  set(validatorsplugin_PART_SRCS ${validatorsplugin_PART_SRCS}
    reportdialog.cpp
    tidy_validator.cpp
  )
endif (LIBTIDY_FOUND)

ki18n_wrap_ui(validatorsplugin_PART_SRCS
  internalvalidator.ui
  remotevalidators.ui
  reportwidget.ui
)

kconfig_add_kcfg_files(validatorsplugin_PART_SRCS settings.kcfgc)

add_library(validatorsplugin MODULE ${validatorsplugin_PART_SRCS})



target_link_libraries(validatorsplugin KF5::Parts KF5::KHtml KF5::WidgetsAddons KF5::KDELibs4Support KF5::IconThemes)
if (LIBTIDY_FOUND)
  target_link_libraries(validatorsplugin ${LIBTIDY_LIBRARIES})
endif (LIBTIDY_FOUND)

install(TARGETS validatorsplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES plugin_validators.rc plugin_validators.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )

install( FILES plugin_validators.rc plugin_validators.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/kwebkitpart/kpartplugins )
install( FILES plugin_validators.rc plugin_validators.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/webenginepart/kpartplugins )

install(FILES validators.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
