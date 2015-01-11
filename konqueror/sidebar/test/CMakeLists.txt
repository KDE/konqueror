########### A test module (not installed by default) ###############
# TODO use it (uninstalled) from the unit test

set(konq_sidebartest_PART_SRCS konqsidebartest.cpp )
add_library(konq_sidebartest MODULE ${konq_sidebartest_PART_SRCS})
target_link_libraries(konq_sidebartest KF5::Parts konqsidebarplugin )
#install(TARGETS konq_sidebartest  DESTINATION ${PLUGIN_INSTALL_DIR} )
#install( FILES test.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqsidebartng/entries )

########### Unit tests ###############

add_executable(sidebar-modulemanagertest modulemanagertest.cpp ../module_manager.cpp)
add_test(sidebar sidebar)
ecm_mark_as_test(sidebar)
target_link_libraries(sidebar-modulemanagertest KF5::KIOCore KF5::KDELibs4Support Qt5::Core Qt5::Test)

