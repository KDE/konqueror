include_directories( ${CMAKE_SOURCE_DIR}/libkonq )

########### konqmimedatatest ###############

kde4_automoc(konqmimedatatest.cpp)

kde4_add_unit_test(konqmimedatatest konqmimedatatest.cpp)

target_link_libraries(konqmimedatatest konq ${QT_QTTEST_LIBRARY})

########### konqundomanager ###############

kde4_automoc(konqundomanagertest.cpp)

kde4_add_unit_test(konqundomanagertest konqundomanagertest.cpp)

target_link_libraries(konqundomanagertest konq ${QT_QTTEST_LIBRARY})

########### favicontest ###############

kde4_automoc(favicontest.cpp)

kde4_add_unit_test(favicontest favicontest.cpp)

target_link_libraries(favicontest konq ${QT_QTTEST_LIBRARY})

########### historymgrtest ###############

kde4_automoc(historymgrtest.cpp)

kde4_add_unit_test(historymgrtest historymgrtest.cpp)

target_link_libraries(historymgrtest konq ${QT_QTTEST_LIBRARY})

############################################
