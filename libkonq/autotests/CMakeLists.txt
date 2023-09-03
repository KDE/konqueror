include(ECMAddTests)

find_package(Qt${KF_MAJOR_VERSION}Test ${QT_MIN_VERSION} CONFIG REQUIRED)


set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )


########### konqpopupmenutest ###############

ecm_add_tests(
   konqpopupmenutest.cpp
   LINK_LIBRARIES KF${KF_MAJOR_VERSION}::Konq Qt${KF_MAJOR_VERSION}::Test
)

############################################
