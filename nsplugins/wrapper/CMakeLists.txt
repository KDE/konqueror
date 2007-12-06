include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )


########### next target ###############

set(nswrapper_LIB_SRCS wrapper.c )


kde4_add_library(nswrapper SHARED ${nswrapper_LIB_SRCS})

target_link_libraries(nswrapper  ${KDE4_KDECORE_LIBS} )

set_target_properties(nswrapper PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS nswrapper  DESTINATION ${LIB_INSTALL_DIR} )

