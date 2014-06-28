set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(Qt5Test CONFIG REQUIRED)

include(ECMAddTests)

# For konqpopupmenutest
remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)

########### konqmimedatatest ###############

ecm_add_tests(
   konqmimedatatest.cpp
   LINK_LIBRARIES Qt5::Test konq
)

########### konqpopupmenutest ###############

ecm_add_tests(
   konqpopupmenutest.cpp
   LINK_LIBRARIES Qt5::Test konq
)

########### favicontest ###############

set(favicontest_SRCS favicontest.cpp)
qt5_add_dbus_interface(favicontest_SRCS ${LibKonq_SOURCE_DIR}/src/favicons/org.kde.FavIcon.xml favicon_interface)

add_executable(favicontest ${favicontest_SRCS})

ecm_mark_as_test(favicontest)

target_link_libraries(favicontest konq Qt5::Test)

############################################
