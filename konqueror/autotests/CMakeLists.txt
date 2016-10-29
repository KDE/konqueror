if (NOT WIN32)

find_package(Qt5Test ${QT_MIN_VERSION} CONFIG REQUIRED)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/..  )

########### konqviewmgrtest ###############

add_executable(konqviewmgrtest konqviewmgrtest.cpp)
add_test(konqviewmgrtest konqviewmgrtest)
ecm_mark_as_test(konqviewmgrtest)
target_link_libraries(konqviewmgrtest kdeinit_konqueror Qt5::Core Qt5::Gui
   # KF5::KHtml
   kwebenginepartlib Qt5::WebEngineWidgets Qt5::Test)

########### historymanagertest ###############

add_executable(historymanagertest historymanagertest.cpp)
add_test(historymanagertest historymanagertest)
ecm_mark_as_test(historymanagertest)
target_link_libraries(historymanagertest KF5::Konq konquerorprivate  Qt5::Core Qt5::Test)

########### undomanagertest ###############

add_executable(undomanagertest undomanagertest.cpp)
add_test(undomanagertest undomanagertest)
ecm_mark_as_test(undomanagertest)
target_link_libraries(undomanagertest kdeinit_konqueror Qt5::Core Qt5::Test)

########### konqhtmltest ###############

add_executable(konqhtmltest konqhtmltest.cpp)
add_test(konqhtmltest konqhtmltest)
ecm_mark_as_test(konqhtmltest)
target_link_libraries(konqhtmltest kdeinit_konqueror kwebenginepartlib Qt5::Core Qt5::Test)

########### konqviewtest ###############

add_executable(konqviewtest konqviewtest.cpp)
add_test(konqviewtest konqviewtest)
ecm_mark_as_test(konqviewtest)
target_link_libraries(konqviewtest kdeinit_konqueror Qt5::Core Qt5::Test)

endif (NOT WIN32)
