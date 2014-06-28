cmake_minimum_required(VERSION 2.8.12)

project(LibKonq)

# ECM setup
find_package(ECM 0.0.14 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

set(LIBKONQ_VERSION "4.97.0")

include(ECMSetupVersion)
include(GenerateExportHeader)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

ecm_setup_version(${LIBKONQ_VERSION} VARIABLE_PREFIX KONQ
                  VERSION_HEADER "${LibKonq_BINARY_DIR}/konq_version.h"
                  SOVERSION 6
)

# Build dependencies
find_package(Qt5 CONFIG REQUIRED COMPONENTS Core DBus Widgets)
find_package(KF5 REQUIRED COMPONENTS Bookmarks KDELibs4Support Parts)

find_package(ZLIB)
set_package_properties(ZLIB PROPERTIES DESCRIPTION "Support for gzip compressed files and data streams"
                       URL "http://www.zlib.net"
                       TYPE REQUIRED
                       PURPOSE "Required for building Konqueror"
                      )

add_subdirectory(src)
if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()

# TODO Remove the if/endif lines if LibKonq is split.
if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
