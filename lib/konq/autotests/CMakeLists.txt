set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

find_package(Qt5Test CONFIG REQUIRED)

include(ECMAddTests)

########### konqpopupmenutest ###############

ecm_add_tests(
   konqpopupmenutest.cpp
   LINK_LIBRARIES KF5Konq Qt5::Test
)

############################################
