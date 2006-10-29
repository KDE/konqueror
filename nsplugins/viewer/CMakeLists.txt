include_directories( ${CMAKE_SOURCE_DIR}/nsplugins )


########### next target ###############

set(nspluginviewer_SRCS nsplugin.cpp viewer.cpp dbusadaptors.cpp callback_proxy.cpp)

kde4_automoc(${nspluginviewer_SRCS})



kde4_add_executable(nspluginviewer ${nspluginviewer_SRCS})

target_link_libraries(nspluginviewer  ${KDE4_KPARTS_LIBS} ${X11_Xt_LIB} )

install(TARGETS nspluginviewer DESTINATION ${BIN_INSTALL_DIR})


