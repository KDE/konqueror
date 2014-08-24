set(kcm_kurifilt_PART_SRCS main.cpp )

add_library(kcm_kurifilt MODULE ${kcm_kurifilt_PART_SRCS})

target_link_libraries(kcm_kurifilt KF5::KIOCore)

install(TARGETS kcm_kurifilt DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES ebrowsing.desktop DESTINATION  ${SERVICES_INSTALL_DIR})
