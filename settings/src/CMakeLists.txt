########### libkonqsettings ###############

set(konqsettings_LIB_SRCS
  settingsmanager.cpp
)

ecm_qt_declare_logging_category(konqsettings_LIB_SRCS HEADER libkonqsettings_debug.h IDENTIFIER LIBKONQSETTINGS_LOG CATEGORY_NAME org.kde.libkonqsettings)

add_library(KF${KF_MAJOR_VERSION}KonqSettings ${konqsettings_LIB_SRCS})

generate_export_header(KF${KF_MAJOR_VERSION}KonqSettings BASE_NAME libkonqsettings EXPORT_FILE_NAME libkonqsettings_export.h)

target_link_libraries(KF${KF_MAJOR_VERSION}KonqSettings
    PUBLIC
    Qt${KF_MAJOR_VERSION}::Core
    KF${KF_MAJOR_VERSION}::ConfigCore
)

set_target_properties(KF${KF_MAJOR_VERSION}KonqSettings PROPERTIES
    VERSION ${KONQ_VERSION}
    SOVERSION ${KONQ_SOVERSION}
    EXPORT_NAME KonqSettings
)

install(TARGETS KF${KF_MAJOR_VERSION}KonqSettings EXPORT KF${KF_MAJOR_VERSION}KonqTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(FILES
  settingsmanager.h
  ${LibKonqSettings_BINARY_DIR}/src/libkonqsettings_export.h
  DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF} COMPONENT Devel)


