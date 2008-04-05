set(kcm_kurifilt_PART_SRCS main.cpp )

kde4_add_plugin(kcm_kurifilt ${kcm_kurifilt_PART_SRCS})

target_link_libraries(kcm_kurifilt ${KDE4_KIO_LIBS})

install(TARGETS kcm_kurifilt DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ebrowsing.desktop DESTINATION  ${SERVICES_INSTALL_DIR})
