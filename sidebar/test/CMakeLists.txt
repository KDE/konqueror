########### A test module (not installed by default) ###############
# TODO use it (uninstalled) from the unit test

find_package(Qt${KF_MAJOR_VERSION} REQUIRED COMPONENTS Test)
include(ECMAddTests)

set(konq_sidebartest_PART_SRCS konqsidebartest.cpp )
add_library(konq_sidebartest MODULE ${konq_sidebartest_PART_SRCS})
target_link_libraries(konq_sidebartest KF${KF_MAJOR_VERSION}::Parts konqsidebarplugin )
#install(TARGETS konq_sidebartest  DESTINATION ${KDE_INSTALL_PLUGINDIR} )
#install( FILES test.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/konqsidebartng/entries )

########### Unit tests ###############

ecm_add_test(
        modulemanagertest.cpp
        ../module_manager.cpp
        ../sidebar_debug.cpp
    LINK_LIBRARIES
        KF${KF_MAJOR_VERSION}::ConfigCore
        KF${KF_MAJOR_VERSION}::Service
        KF${KF_MAJOR_VERSION}::KIOCore
        Qt${KF_MAJOR_VERSION}::Core
        Qt${KF_MAJOR_VERSION}::Test
    TEST_NAME sidebar-modulemanagertest
)
target_include_directories(sidebar-modulemanagertest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..)

