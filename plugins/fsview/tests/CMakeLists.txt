include (ECMMarkAsTest)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
include_directories( ${CMAKE_CURRENT_BINARY_DIR}/.. )

########### next target ###############

set(libfsview_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../treemap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../fsview.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../scan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../inode.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/../fsviewdebug.cpp
    )


set(scantest_SRCS scantest.cpp ${libfsview_SRCS})

add_executable(scantest ${scantest_SRCS})
ecm_mark_as_test(scantest)

target_link_libraries(scantest
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::IconThemes
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::ConfigCore
    KF${KF_MAJOR_VERSION}::WidgetsAddons
    Qt${KF_MAJOR_VERSION}::Widgets)
