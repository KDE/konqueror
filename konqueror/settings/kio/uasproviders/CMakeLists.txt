FILE(GLOB useragents *.desktop)
install(FILES ${useragents} DESTINATION
    ${SERVICES_INSTALL_DIR}/useragentstrings)
