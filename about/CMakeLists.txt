
project(kioslave-about)



########### next target ###############

set(kio_about_PART_SRCS kio_about.cpp )

kde4_automoc(${kio_about_PART_SRCS})

kde4_add_plugin(kio_about ${kio_about_PART_SRCS})


target_link_libraries(kio_about  ${KDE4_KIO_LIBS} )

install(TARGETS kio_about  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES about.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
