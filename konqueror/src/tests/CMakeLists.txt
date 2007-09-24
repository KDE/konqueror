set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_SOURCE_DIR}/libkonq ${CMAKE_CURRENT_SOURCE_DIR}/.. ${KDE4_INCLUDES} )

########### centralwidget ###############


kde4_add_executable(centralwidget TEST centralwidget.cpp)

target_link_libraries(centralwidget konq ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

########### konqviewmgrtest ###############


kde4_add_executable(konqviewmgrtest TEST konqviewmgrtest.cpp)

target_link_libraries(konqviewmgrtest kdeinit_konqueror ${QT_QTTEST_LIBRARY})

add_test(konqueror-viewmgrtest ${CMAKE_CURRENT_BINARY_DIR}/konqviewmgrtest)

############################################
