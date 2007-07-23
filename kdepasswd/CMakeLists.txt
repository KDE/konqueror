

add_subdirectory( kcm ) 

########### next target ###############

set(kdepasswd_SRCS kdepasswd.cpp passwd.cpp passwddlg.cpp )


kde4_add_executable(kdepasswd ${kdepasswd_SRCS})

target_link_libraries(kdepasswd  ${KDE4_KIO_LIBS} )

install(TARGETS kdepasswd DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES kdepasswd.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

