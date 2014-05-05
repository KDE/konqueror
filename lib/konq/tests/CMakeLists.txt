set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

########### konqmimedatatest ###############

kde4_add_unit_test(konqmimedatatest konqmimedatatest.cpp)

target_link_libraries(konqmimedatatest konq KF5::KDELibs4Support  Qt5::Gui Qt5::Core
                      Qt5::Test)

########### konqpopupmenutest ###############

kde4_add_unit_test(konqpopupmenutest konqpopupmenutest.cpp)

target_link_libraries(konqpopupmenutest konq KF5::KDELibs4Support
                     Qt5::Core Qt5::Gui Qt5::Test)

########### favicontest ###############

set(favicontest_SRCS favicontest.cpp)
qt5_add_dbus_interface(favicontest_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../favicons/org.kde.FavIcon.xml favicon_interface)
kde4_add_unit_test(favicontest ${favicontest_SRCS})

target_link_libraries(favicontest konq KF5::KDELibs4Support Qt5::Core
                      Qt5::Gui Qt5::DBus Qt5::Test)

############################################
