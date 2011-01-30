add_subdirectory(babel)
add_subdirectory(dirfilter)
add_subdirectory(domtreeviewer)
add_subdirectory(fsview)
add_subdirectory(imgallery)
add_subdirectory(khtmlsettings)
add_subdirectory(uachanger)
add_subdirectory(validators)
add_subdirectory(webarchiver)
########### install files ###############

kde4_create_handbook(index.docbook INSTALL_DESTINATION ${HTML_INSTALL_DIR}/en SUBDIR konq-plugins)
