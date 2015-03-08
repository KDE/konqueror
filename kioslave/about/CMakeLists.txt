cmake_minimum_required(VERSION 2.8.12)

project(KIOSlave-About)

# ECM setup
find_package(ECM 1.7.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

# Build dependencies
find_package(KF5 REQUIRED COMPONENTS KIO)

########### next target ###############

set(kio_about_PART_SRCS kio_about.cpp)


add_library(kio_about MODULE ${kio_about_PART_SRCS})


target_link_libraries(kio_about
  KF5::KIOCore
)

install(TARGETS kio_about DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install( FILES about.protocol DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

if("${CMAKE_SOURCE_DIR}" STREQUAL "${KIOSlave-About_SOURCE_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
