########### libkonq ###############

set(konqsettings_LIB_SRCS
    konqsettingsbase.cpp
)

kconfig_add_kcfg_files(konqsettings_LIB_SRCS konqsettings.kcfgc)

ecm_qt_declare_logging_category(konqsettings_LIB_SRCS HEADER libkonqsettings_debug.h IDENTIFIER LIBKONQSETTINGS_LOG CATEGORY_NAME org.kde.libkonqsettings)

add_library(KF${KF_MAJOR_VERSION}KonqSettings ${konqsettings_LIB_SRCS})

generate_export_header(KF${KF_MAJOR_VERSION}KonqSettings BASE_NAME libkonqsettings EXPORT_FILE_NAME libkonqsettings_export.h)

add_library(KF${KF_MAJOR_VERSION}::KonqSettings ALIAS KF${KF_MAJOR_VERSION}KonqSettings)

target_link_libraries(KF${KF_MAJOR_VERSION}KonqSettings
    PUBLIC
    KF${KF_MAJOR_VERSION}::ConfigCore
    KF${KF_MAJOR_VERSION}::ConfigGui
    KF${KF_MAJOR_VERSION}::Completion
)

set_target_properties(KF${KF_MAJOR_VERSION}KonqSettings PROPERTIES
    VERSION ${KONQSETTINGS_VERSION}
    SOVERSION ${KONQSETTINGS_SOVERSION}
    EXPORT_NAME KonqSettings
)

install(TARGETS KF${KF_MAJOR_VERSION}KonqSettings EXPORT KF${KF_MAJOR_VERSION}KonqSettingsTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(FILES
    ${LibKonqSettings_BINARY_DIR}/src/konqsettings.h
    ${LibKonqSettings_BINARY_DIR}/src/libkonqsettings_export.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}
    COMPONENT Devel
)

install( FILES konqueror.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
