if (NOT WIN32)

find_package(Qt${KF_MAJOR_VERSION}Test ${QT_MIN_VERSION} CONFIG REQUIRED)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/..  )

########### konqviewmgrtest ###############

ecm_add_test(konqviewmgrtest.cpp
    LINK_LIBRARIES kdeinit_konqueror Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Gui kwebenginepartlib Qt${KF_MAJOR_VERSION}::WebEngineWidgets Qt${KF_MAJOR_VERSION}::Test)

########### historymanagertest ###############

ecm_add_test(historymanagertest.cpp
    LINK_LIBRARIES KF${KF_MAJOR_VERSION}::Konq konquerorprivate Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Test)

########### undomanagertest ###############

ecm_add_test(undomanagertest.cpp
    LINK_LIBRARIES kdeinit_konqueror Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Test)

########### konqhtmltest ###############

ecm_add_test(konqhtmltest.cpp
    LINK_LIBRARIES kdeinit_konqueror kwebenginepartlib Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Test)

########### konqviewtest ###############

ecm_add_test(konqviewtest.cpp
    LINK_LIBRARIES kdeinit_konqueror Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Test)

########### openorsavequestion_unittest ###############

ecm_add_test(openorsavequestion_unittest.cpp
    LINK_LIBRARIES kdeinit_konqueror Qt${KF_MAJOR_VERSION}::Core Qt${KF_MAJOR_VERSION}::Test)

endif (NOT WIN32)
