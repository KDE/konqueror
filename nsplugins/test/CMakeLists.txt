if(KDE4_BUILD_TESTS)
include_directories( ${CMAKE_SOURCE_DIR}/libltdl ${CMAKE_CURRENT_BINARY_DIR}/..  )


########### next target ###############

set(testnsplugin_SRCS testnsplugin.cpp )

kde4_automoc(${testnsplugin_SRCS})

kde4_add_executable(testnsplugin ${testnsplugin_SRCS})

target_link_libraries(testnsplugin  ${KDE4_KDEUI_LIBS} nsplugin )

endif(KDE4_BUILD_TESTS)

