add_subdirectory( pics )
add_subdirectory( favicons )
add_subdirectory( Templates )
add_subdirectory( tests )

# For crc32 in konq_historymgr.cc
include_directories( ${ZLIB_INCLUDE_DIR}  )

########### next target ###############

# Please use .cpp for new files
set(konq_LIB_SRCS
   konq_popupmenu.cpp       # OK to remain in libkonq, was needed by kicker applets like the trash applet
   knewmenu.cpp             # used by dolphin, KonqPopupMenu, and konqueror (File menu)
   konq_menuactions.cpp     # used by dolphin, KonqPopupMenu
   konq_operations.cpp
   konq_events.cpp
   konq_undo.cc
   konq_historymgr.cc       # used by sidebar/trees/history_module and konqueror
   konq_historyentry.cpp
   konq_filetip.cc          # unused right now, but should be used by dolphin_part in the future.
                            # Also useful to keep separate? (e.g. in kde3  kerry used it too)
   konqmimedata.cpp
)

qt4_add_dbus_adaptor( konq_LIB_SRCS org.kde.libkonq.KonqHistoryManager.xml konq_historymgr.h KonqHistoryManager )
qt4_add_dbus_adaptor( konq_LIB_SRCS org.kde.libkonq.UndoManager.xml konq_undo.h KonqUndoManager) 

kde4_add_library(konq SHARED ${konq_LIB_SRCS})

target_link_libraries(konq ${QT_QTXML_LIBRARY} ${KDE4_KPARTS_LIBS})

set_target_properties(konq PROPERTIES VERSION ${KDE_NON_GENERIC_LIB_VERSION} SOVERSION ${KDE_NON_GENERIC_LIB_SOVERSION} )
install(TARGETS konq 
        RUNTIME DESTINATION ${BIN_INSTALL_DIR}
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

########### next target ###############

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
 konq_defaults.h
 konq_operations.h
 libkonq_export.h
 konq_events.h
 konq_undo.h
 konq_historymgr.h
 konq_historyentry.h
 konq_filetip.h
 konqmimedata.h
DESTINATION ${INCLUDE_INSTALL_DIR}
)
install( FILES konqpopupmenuplugin.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )

install( FILES org.kde.libkonq.KonqHistoryManager.xml org.kde.libkonq.UndoManager.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

