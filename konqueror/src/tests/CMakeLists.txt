set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${KDE4_INCLUDES} )

########### centralwidget ###############

kde4_add_executable(centralwidget TEST centralwidget.cpp)
target_link_libraries(centralwidget konq ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})

########### konqviewmgrtest ###############

kde4_add_unit_test(konqviewmgrtest TEST konqviewmgrtest.cpp)
target_link_libraries(konqviewmgrtest kdeinit_konqueror ${QT_QTTEST_LIBRARY})

########### historymanagertest ###############

kde4_add_unit_test(historymanagertest historymanagertest.cpp)
target_link_libraries(historymanagertest konquerorprivate ${QT_QTTEST_LIBRARY})

########### undomanagertest ###############

kde4_add_unit_test(undomanagertest undomanagertest.cpp)
target_link_libraries(undomanagertest kdeinit_konqueror ${QT_QTTEST_LIBRARY})
