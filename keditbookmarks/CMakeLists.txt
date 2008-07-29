########### next target ###############

set(kbookmarkmerger_SRCS kbookmarkmerger.cpp )


kde4_add_executable(kbookmarkmerger ${kbookmarkmerger_SRCS})

target_link_libraries(kbookmarkmerger  ${KDE4_KIO_LIBS})
if (Q_WS_X11)
  target_link_libraries(kbookmarkmerger ${X11_X11_LIB})
endif (Q_WS_X11)

install(TARGETS kbookmarkmerger ${INSTALL_TARGETS_DEFAULT_ARGS})


########### next target ###############

set(keditbookmarks_KDEINIT_SRCS 
   main.cpp 
   toplevel.cpp 
   actionsimpl.cpp 
   commands.cpp 
   importers.cpp 
   bookmarkiterator.cpp 
   testlink.cpp 
   favicons.cpp 
   updater.cpp 
   exporters.cpp 
   bookmarkinfo.cpp 
   kebsearchline.cpp 
   bookmarkmodel.cpp 
   bookmarklistview.cpp 
   treeitem.cpp )

set(toplevel_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.keditbookmarks.xml)
qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/toplevel.h org.kde.keditbookmarks.xml )

qt4_add_dbus_interface( keditbookmarks_KDEINIT_SRCS ${toplevel_xml} toplevel_interface )
qt4_add_dbus_interface( keditbookmarks_KDEINIT_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/konq/favicons/org.kde.FavIcon.xml favicon_interface)


kde4_add_kcfg_files(keditbookmarks_KDEINIT_SRCS settings.kcfgc )

kde4_add_kdeinit_executable( keditbookmarks ${keditbookmarks_KDEINIT_SRCS})

target_link_libraries(kdeinit_keditbookmarks  ${KDE4_KDE3SUPPORT_LIBS} konq ${KDE4_KPARTS_LIBS})
if (Q_WS_X11)
  target_link_libraries(kdeinit_keditbookmarks ${X11_X11_LIB})
endif (Q_WS_X11)

install(TARGETS kdeinit_keditbookmarks  ${INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS keditbookmarks          ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES keditbookmarks.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES keditbookmarksui.rc  keditbookmarks-genui.rc  DESTINATION  ${DATA_INSTALL_DIR}/keditbookmarks )
