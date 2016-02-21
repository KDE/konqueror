cmake_minimum_required(VERSION 2.8.12)

project(LibKonq)
add_definitions(-DTRANSLATION_DOMAIN="libkonq")

# ECM setup
find_package(ECM 1.7.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

set(LIBKONQ_VERSION "5.97.0")

include(ECMSetupVersion)
include(GenerateExportHeader)
include(ECMPackageConfigHelpers)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

ecm_setup_version(${LIBKONQ_VERSION} VARIABLE_PREFIX KONQ
                  VERSION_HEADER "${LibKonq_BINARY_DIR}/konq_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5KonqConfigVersion.cmake"
                  SOVERSION 6
)

# Build dependencies
find_package(Qt5 CONFIG REQUIRED COMPONENTS Core DBus Widgets)
find_package(KF5 REQUIRED COMPONENTS Bookmarks Parts)

find_package(ZLIB)
set_package_properties(ZLIB PROPERTIES DESCRIPTION "Support for gzip compressed files and data streams"
                       URL "http://www.zlib.net"
                       TYPE REQUIRED
                       PURPOSE "Required for building Konqueror"
                      )

# Targets
add_subdirectory(src)
if(BUILD_TESTING)
  find_package(Qt5Test ${QT_MIN_VERSION} CONFIG REQUIRED)
  add_subdirectory(autotests)
endif()

# CMake files
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5Konq")

ecm_configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KF5KonqConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5KonqConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF5KonqConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5KonqConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KF5KonqTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KF5KonqTargets.cmake
    NAMESPACE KF5::
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/konq_version.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}
    COMPONENT Devel
)

# TODO Remove the if/endif lines if LibKonq is split.
if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
