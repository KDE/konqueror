
add_definitions(-D_LARGEFILE64_SOURCE )

if(Q_WS_X11)
  find_package(X11 REQUIRED) # both subdirs require the X11 libs
endif(Q_WS_X11)

add_subdirectory( src )
add_subdirectory( client )

add_subdirectory( about )
add_subdirectory( pics )
add_subdirectory( sidebar )
add_subdirectory( preloader )
add_subdirectory( settings )

########### install files ###############
install( PROGRAMS
   kfmclient.desktop kfmclient_dir.desktop kfmclient_html.desktop kfmclient_war.desktop
   konqbrowser.desktop konquerorsu.desktop Home.desktop
   DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install(FILES konqueror.appdata.xml DESTINATION ${SHARE_INSTALL_PREFIX}/appdata )

# Make sure kfmclient can find konqueror.desktop even if people remove it from the K menu (#62242)
# This is why konqueror.desktop is installed into services.
install( FILES konqueror.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

install(FILES profile_webbrowsing.desktop    DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME webbrowsing)
install(FILES profile_filemanagement.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME filemanagement)
install(FILES profile_tabbedbrowsing.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME tabbedbrowsing)
install(FILES profile_kde_devel.desktop      DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME kde_devel)

if(NOT WIN32)
   install(FILES profile_midnightcommander.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME midnightcommander)
endif(NOT WIN32)
