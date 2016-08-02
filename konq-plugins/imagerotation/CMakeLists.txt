
#if(NOT WIN32)
#  install( FILES jpegorient.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/ServiceMenus )
#  install( PROGRAMS exif.py jpegorient orient.py DESTINATION ${KDE_INSTALL_BINDIR} )
#endif(NOT WIN32)

install( FILES imageconverter.desktop  DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/ServiceMenus )


