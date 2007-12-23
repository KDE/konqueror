set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${KDE4_INCLUDES} )

########### filetypestest ###############

kde4_add_unit_test(filetypestest TEST filetypestest.cpp ../mimetypedata.cpp)
target_link_libraries(filetypestest kdeinit_konqueror ${QT_QTTEST_LIBRARY})
