if (NOT WIN32)
find_package(Qt${KF_MAJOR_VERSION}Test ${QT_MIN_VERSION} CONFIG REQUIRED)

add_compile_definitions(BUILD_TESTING)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/..  )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/downloadactionquestiontest/  )

########### konqviewmgrtest ###############

ecm_add_test(konqviewmgrtest.cpp TEST_NAME konqviewmgrtest
    LINK_LIBRARIES konqueror_internal_lib Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Gui kwebenginepartlib Qt${KF_MAJOR_VERSION}::WebEngineWidgets Qt${KF_MAJOR_VERSION}::Test)
qt_add_resources(konqviewmgrtest "viewmgrtest.qrc" PREFIX "/kxmlgui5/konqviewmgrtest/" BASE ../src/ FILES ../src/konqueror.rc )

########### historymanagertest ###############

ecm_add_test(historymanagertest.cpp
    LINK_LIBRARIES KF${KF_MAJOR_VERSION}::Konq konquerorprivate Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Test)

########### undomanagertest ###############

ecm_add_test(undomanagertest.cpp
    LINK_LIBRARIES konqueror_internal_lib Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Test)

########### konqhtmltest ###############

set (konqhtmltest_SRCS konqhtmltest/konqhtmltest.cpp)
qt_add_resources(konqhtmltest_SRCS konqhtmltest/konqhtmltest.qrc)
ecm_add_test(${konqhtmltest_SRCS} TEST_NAME konqhtmltest
    LINK_LIBRARIES konqueror_internal_lib kwebenginepartlib Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Test)

########### konqviewtest ###############

ecm_add_test(konqviewtest.cpp
    LINK_LIBRARIES konqueror_internal_lib Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Test)

########### downloadactionquestiontest ###############

ecm_add_test(downloadactionquestiontest/downloadactionquestiontest.cpp downloadactionquestiontest/downloadactionquestiontestjsonloader.cpp ../src/downloadactionquestion.cpp
    TEST_NAME downloadactionquestiontest
    LINK_LIBRARIES konqueror_internal_lib Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Test KF${KF_MAJOR_VERSION}::Service)
    add_compile_definitions(BUILD_DOWNLOAD_ACTION_QUESTION_TESTS TRANSLATION_DOMAIN=\"konqueror\")
qt_add_resources(downloadactionquestiontest PREFIX "/" FILES ./downloadactionquestiontest/downloadactionquestion_testdata.json BASE ./downloadactionquestiontest)

endif (NOT WIN32)
