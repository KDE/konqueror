########### A test module (not installed by default) ###############
# TODO use it (uninstalled) from the unit test

find_package(Qt5 REQUIRED COMPONENTS Test)
include(ECMAddTests)

set(konq_sidebartest_PART_SRCS konqsidebartest.cpp )
add_library(konq_sidebartest MODULE ${konq_sidebartest_PART_SRCS})
target_link_libraries(konq_sidebartest KF5::Parts konqsidebarplugin )
#install(TARGETS konq_sidebartest  DESTINATION ${KDE_INSTALL_PLUGINDIR} )
#install( FILES test.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/konqsidebartng/entries )

########### Unit tests ###############

ecm_add_test(
        modulemanagertest.cpp
        ../module_manager.cpp
        ../sidebar_debug.cpp
    LINK_LIBRARIES
        KF5::ConfigCore
        KF5::Service
        KF5::KIOCore
        Qt5::Core
        Qt5::Test
    TEST_NAME sidebar-modulemanagertest
)
target_include_directories(sidebar-modulemanagertest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..)

