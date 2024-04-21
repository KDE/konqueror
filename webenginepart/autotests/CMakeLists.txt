add_definitions(-DBUILD_TESTING)

include(ECMAddTests)

find_package(Qt${KF_MAJOR_VERSION}Test ${QT_MIN_VERSION} CONFIG REQUIRED)

macro(webenginepart_unit_tests)
  foreach(_testname ${ARGN})
    ecm_add_test(${_testname}.cpp TEST_NAME ${_testname}
      LINK_LIBRARIES kwebenginepartlib Qt${KF_MAJOR_VERSION}::Test)
  endforeach()
endmacro(webenginepart_unit_tests)

webenginepart_unit_tests(
  webengine_partapi_test
)
