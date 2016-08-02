include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )


########### next target ###############

set(nswrapper_LIB_SRCS wrapper.c )


add_library(nswrapper SHARED ${nswrapper_LIB_SRCS})

target_link_libraries(nswrapper  KF5::KDELibs4Support )

set_target_properties(nswrapper PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS nswrapper  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

