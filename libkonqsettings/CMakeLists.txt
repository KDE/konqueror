project(LibKonqSettings)

set(LIBKONQSETTINGS_VERSION "5.97.0")

ecm_setup_version(${LIBKONQSETTINGS_VERSION} VARIABLE_PREFIX KONQSETTINGS
                  VERSION_HEADER "${LibKonqSettings_BINARY_DIR}/konqsettings_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF${KF_MAJOR_VERSION}KonqSettingsConfigVersion.cmake"
                  SOVERSION 7
)

# Build dependencies
find_package(Qt${KF_MAJOR_VERSION} CONFIG REQUIRED COMPONENTS Core Widgets)
find_package(KF${KF_MAJOR_VERSION} REQUIRED COMPONENTS Config Completion)

# Targets
add_subdirectory(src)
# if(BUILD_TESTING)
#   add_subdirectory(autotests)
# endif()

# CMake files
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF${KF_MAJOR_VERSION}Konq")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KonqSettingsConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF${KF_MAJOR_VERSION}KonqSettingsConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF${KF_MAJOR_VERSION}KonqSettingsConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF${KF_MAJOR_VERSION}KonqSettingsConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KF${KF_MAJOR_VERSION}KonqSettingsTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KF${KF_MAJOR_VERSION}KonqSettingsTargets.cmake
    NAMESPACE KF${KF_MAJOR_VERSION}::
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/konqsettings_version.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}
    COMPONENT Devel
)
