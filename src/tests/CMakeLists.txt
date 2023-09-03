set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/..  )

########### centralwidget ###############

include(ECMMarkAsTest)
add_executable(centralwidget centralwidget.cpp)
ecm_mark_as_test(centralwidget)
target_link_libraries(centralwidget
   Qt${KF_MAJOR_VERSION}::Widgets
)

