project(konqueror)

add_definitions(-D_LARGEFILE64_SOURCE )

add_definitions (-DQT3_SUPPORT)

add_subdirectory( src )
add_subdirectory( client )

add_subdirectory( about )
add_subdirectory( pics )
add_subdirectory( sidebar )
add_subdirectory( preloader )
add_subdirectory( quickprint )
add_subdirectory( remoteencodingplugin )
# Replaced by dolphinpart
#add_subdirectory( dirpart )
add_subdirectory( settings )
add_subdirectory( kttsplugin )

if(UNIX)
   add_subdirectory( shellcmdplugin )
endif(UNIX)

########### install files ###############
install( FILES konqfilemgr.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kfmclient.desktop kfmclient_dir.desktop kfmclient_html.desktop  	kfmclient_war.desktop konqbrowser.desktop konquerorsu.desktop Home.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES konqueror.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

install(FILES profile_webbrowsing.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME webbrowsing)
install(FILES profile_filemanagement.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME filemanagement)
if(NOT WIN32)
install(FILES profile_midnightcommander.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME midnightcommander)
endif(NOT WIN32)
install(FILES profile_tabbedbrowsing.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME tabbedbrowsing)
install(FILES profile_kde_devel.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME kde_devel)
install(FILES profile_simplebrowser.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/profiles/ RENAME simplebrowser)
