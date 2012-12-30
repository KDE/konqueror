
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

macro_optional_find_package(LibTidy)
set_package_properties(LibTidy PROPERTIES DESCRIPTION "HTML Tidy"
                       URL "http://tidy.sourceforge.net"
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

kde4_add_ui_files(validatorsplugin_PART_SRCS
  internalvalidator.ui
  remotevalidators.ui
  reportwidget.ui
)

kde4_add_kcfg_files(validatorsplugin_PART_SRCS settings.kcfgc)

kde4_add_plugin(validatorsplugin ${validatorsplugin_PART_SRCS})



target_link_libraries(validatorsplugin ${KDE4_KPARTS_LIBS} ${KDE4_KHTML_LIBRARY})
if (LIBTIDY_FOUND)
  target_link_libraries(validatorsplugin ${LIBTIDY_LIBRARIES})
endif (LIBTIDY_FOUND)

install(TARGETS validatorsplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES plugin_validators.rc plugin_validators.desktop  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )

install( FILES plugin_validators.rc plugin_validators.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart/kpartplugins )

install(FILES validators.kcfg DESTINATION ${KCFG_INSTALL_DIR})
