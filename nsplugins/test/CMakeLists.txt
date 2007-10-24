set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_SOURCE_DIR}/libltdl ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../ )

########### next target ###############

set(testnsplugin_SRCS testnsplugin.cpp ${nsplugins_SRCS})

kde4_add_executable(testnsplugin TEST ${testnsplugin_SRCS})

target_link_libraries(testnsplugin   ${KDE4_KDE3SUPPORT_LIBS} ${QT_QT3SUPPORT_LIBRARY} ${QT_QTGUI_LIBRARY}  )


