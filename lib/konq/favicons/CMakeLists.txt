set(kded_favicons_SRCS favicons.cpp )

qt4_add_dbus_adaptor(kded_favicons_SRCS org.kde.FavIcon.xml favicons.h FavIconsModule favicons_adaptor FavIconsAdaptor)

kde4_add_plugin(kded_favicons ${kded_favicons_SRCS})

target_link_libraries( kded_favicons ${KDE4_KIO_LIBS} )


install(TARGETS kded_favicons  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES favicons.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
if(NOT WIN32)
  install( FILES favicons.upd DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
  install( PROGRAMS move_favicons.sh  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
endif(NOT WIN32)

install( FILES org.kde.FavIcon.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
