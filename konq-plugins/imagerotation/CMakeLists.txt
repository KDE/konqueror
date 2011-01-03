
#if(NOT WIN32)
#  install( FILES jpegorient.desktop DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus )
#  install( PROGRAMS exif.py jpegorient orient.py DESTINATION ${BIN_INSTALL_DIR} )
#endif(NOT WIN32)

install( FILES imageconverter.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus )


