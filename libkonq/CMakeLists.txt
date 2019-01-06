project(LibKonq)
add_definitions(-DTRANSLATION_DOMAIN="libkonq")

set(LIBKONQ_VERSION "5.97.0")

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
                       URL "https://www.zlib.net"
                       TYPE REQUIRED
                       PURPOSE "Required for building LibKonq"
                      )

# Targets
add_subdirectory(src)
if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()

# CMake files
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5Konq")

configure_package_config_file(
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
