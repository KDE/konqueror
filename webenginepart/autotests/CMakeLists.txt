add_definitions(-DBUILD_TESTING)

include(ECMAddTests)

find_package(Qt5Test ${QT_MIN_VERSION} CONFIG REQUIRED)

macro(webenginepart_unit_tests)
  foreach(_testname ${ARGN})
    ecm_add_test(${_testname}.cpp TEST_NAME ${_testname}
      LINK_LIBRARIES kwebenginepartlib Qt5::Test)
  endforeach()
endmacro(webenginepart_unit_tests)

webenginepart_unit_tests(
  webengine_partapi_test
)

if (NOT(ManageCookiesInternally))
  webenginepart_unit_tests(
    webenginepartcookiejarkio_test
  )
  target_link_libraries(webenginepartcookiejarkio_test Qt5::DBus)
endif()

