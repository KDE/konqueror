set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )


include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

########### next target ###############

set(libfsview_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../treemap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../fsview.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../scan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../inode.cpp
    )


set(scantest_SRCS scantest.cpp ${libfsview_SRCS})

add_executable(scantest ${scantest_SRCS})
ecm_mark_as_test(scantest)

target_link_libraries(scantest  KF5::KIOCore KF5::KDELibs4Support Qt5::Widgets)


