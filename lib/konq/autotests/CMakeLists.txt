set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(Qt5Test CONFIG REQUIRED)

include(ECMAddTests)

# For konqpopupmenutest
remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)

########### konqpopupmenutest ###############

ecm_add_tests(
   konqpopupmenutest.cpp
   LINK_LIBRARIES KF5Konq Qt5::Test
)

############################################
