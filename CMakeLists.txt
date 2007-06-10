add_subdirectory( dolphin )
add_subdirectory( kwrite )
add_subdirectory( kdialog )
add_subdirectory( keditbookmarks )
add_subdirectory( konqueror )
add_subdirectory( kfind )
add_subdirectory( kappfinder )

if ( Q_WS_MAC )
  add_subdirectory( konsole )
  add_subdirectory( kdepasswd )
endif ( Q_WS_MAC )

if ( Q_WS_X11 )
  add_subdirectory( nsplugins )
  add_subdirectory( konsole )
  add_subdirectory( kdepasswd )
endif ( Q_WS_X11 )


