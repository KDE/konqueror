if (NOT WIN32)

find_package(Qt5Test ${QT_MIN_VERSION} CONFIG REQUIRED)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/..  )

########### konqviewmgrtest ###############

ecm_add_test(konqviewmgrtest.cpp
    LINK_LIBRARIES kdeinit_konqueror Qt5::Core Qt5::Gui kwebenginepartlib Qt5::WebEngineWidgets Qt5::Test)

########### historymanagertest ###############

ecm_add_test(historymanagertest.cpp
    LINK_LIBRARIES KF5::Konq konquerorprivate Qt5::Core Qt5::Test)

########### undomanagertest ###############

ecm_add_test(undomanagertest.cpp
    LINK_LIBRARIES kdeinit_konqueror Qt5::Core Qt5::Test)

########### konqhtmltest ###############

ecm_add_test(konqhtmltest.cpp
    LINK_LIBRARIES kdeinit_konqueror kwebenginepartlib Qt5::Core Qt5::Test)

########### konqviewtest ###############

ecm_add_test(konqviewtest.cpp
    LINK_LIBRARIES kdeinit_konqueror Qt5::Core Qt5::Test)

########### openorsavequestion_unittest ###############

ecm_add_test(openorsavequestion_unittest.cpp
    LINK_LIBRARIES kdeinit_konqueror Qt5::Core Qt5::Test)

endif (NOT WIN32)
