set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

########### konqmimedatatest ###############

kde4_add_unit_test(konqmimedatatest konqmimedatatest.cpp)

target_link_libraries(konqmimedatatest konq ${QT_QTTEST_LIBRARY})

########### konqpopupmenutest ###############

kde4_add_unit_test(konqpopupmenutest konqpopupmenutest.cpp)

target_link_libraries(konqpopupmenutest konq ${QT_QTTEST_LIBRARY})

########### konqfileundomanager ###############

kde4_add_unit_test(konqfileundomanagertest konqfileundomanagertest.cpp)

target_link_libraries(konqfileundomanagertest konq ${QT_QTTEST_LIBRARY})

########### favicontest ###############

set(favicontest_SRCS favicontest.cpp)
qt4_add_dbus_interface(favicontest_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../favicons/org.kde.FavIcon.xml favicon_interface)
kde4_add_unit_test(favicontest ${favicontest_SRCS})

target_link_libraries(favicontest konq ${QT_QTTEST_LIBRARY})

########### historymgrtest ###############

kde4_add_unit_test(historymgrtest historymgrtest.cpp)

target_link_libraries(historymgrtest konq ${QT_QTTEST_LIBRARY})

############################################
