add_subdirectory( pics )
add_subdirectory( favicons )
add_subdirectory( Templates )
add_subdirectory( tests )

########### libkonq ###############

# Please use .cpp for new files
set(konq_LIB_SRCS
   konq_popupmenu.cpp       # remains in libkonq, was needed by kicker applets like the trash applet
   konq_popupmenuinformation.cpp # used by KonqPopupMenu, KonqMenuActions and KonqPopupMenuPlugin
   knewmenu.cpp             # used by dolphin, KonqPopupMenu, and konqueror (File menu; to be moved to dolphinpart)
   konq_menuactions.cpp     # used by dolphin, KonqPopupMenu. Move to kdelibs, for kickoff?
   konq_copytomenu.cpp      # used by dolphin, KonqPopupMenu
   konq_operations.cpp      # used by dolphin and konqueror
   konq_events.cpp
   konq_filetip.cc          # unused right now, but should be used by dolphin_part in the future.
                            # Also useful to keep separate? (e.g. in kde3  kerry used it too)
   konqmimedata.cpp         # used by dolphin, KonqOperations, some filemanagement konqueror modules.
)

kde4_add_library(konq SHARED ${konq_LIB_SRCS})

target_link_libraries(konq ${KDE4_KPARTS_LIBS})

set_target_properties(konq PROPERTIES VERSION ${KDE_NON_GENERIC_LIB_VERSION} SOVERSION ${KDE_NON_GENERIC_LIB_SOVERSION} )
install(TARGETS konq  ${INSTALL_TARGETS_DEFAULT_ARGS})

########### konq_sound ###############

set(konq_sound_LIB_SRCS konq_sound.cc)
kde4_add_plugin(konq_sound ${konq_sound_LIB_SRCS})
target_link_libraries(konq_sound ${KDE4_KDECORE_LIBS} ${KDE4_PHONON_LIBS})
install(TARGETS konq_sound  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES directory_bookmarkbar.desktop   DESTINATION  ${DATA_INSTALL_DIR}/kbookmark )
install( FILES
 konq_popupmenu.h
 knewmenu.h
 konq_menuactions.h
 # konq_copytomenu.h - anyone needs it?
 konq_operations.h
 libkonq_export.h
 konq_events.h
 # konq_filetip.h
 konqmimedata.h
DESTINATION ${INCLUDE_INSTALL_DIR}
)
install( FILES konqpopupmenuplugin.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )

