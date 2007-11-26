
# There is no way to use those tiles at the moment
# Maybe dolphin will get "background pixmap" support in the future?
#add_subdirectory( tiles )

add_subdirectory( indicators )
add_subdirectory( actions )


kde4_install_icons( ${ICON_INSTALL_DIR}  )
