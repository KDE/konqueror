project(LibKonq)
add_definitions(-DTRANSLATION_DOMAIN="libkonq")

set(LIBKONQ_VERSION "5.97.0")

ecm_setup_version(${LIBKONQ_VERSION} VARIABLE_PREFIX KONQ
                  VERSION_HEADER "${LibKonq_BINARY_DIR}/konq_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF${KF_MAJOR_VERSION}KonqConfigVersion.cmake"
                  SOVERSION 7
)

# Build dependencies
find_package(Qt${KF_MAJOR_VERSION} CONFIG REQUIRED COMPONENTS Core DBus Widgets)
find_package(KF${KF_MAJOR_VERSION} REQUIRED COMPONENTS Bookmarks Parts)

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
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF${KF_MAJOR_VERSION}Konq")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KonqConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF${KF_MAJOR_VERSION}KonqConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF${KF_MAJOR_VERSION}KonqConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF${KF_MAJOR_VERSION}KonqConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KF${KF_MAJOR_VERSION}KonqTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KF${KF_MAJOR_VERSION}KonqTargets.cmake
    NAMESPACE KF${KF_MAJOR_VERSION}::
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/konq_version.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}
    COMPONENT Devel
)
