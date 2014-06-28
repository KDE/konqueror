add_definitions(-DTRANSLATION_DOMAIN=\"libkonq\")
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

########### libkonq ###############

set(konq_LIB_SRCS
   konq_popupmenu.cpp       # used by konqueror, kfind, folderview, kickoff
   konq_popupmenuplugin.cpp # for KonqPopupMenu and its plugins
   konq_dndpopupmenuplugin.cpp # for KonqDndPopupMenu and its plugins
   konq_copytomenu.cpp         # used by dolphin, KonqPopupMenu
   konq_operations.cpp         # used by dolphin and konqueror
   konq_statusbarmessagelabel.cpp  # used by dolphin and konqueror
   konq_events.cpp
   konqmimedata.cpp         # used by dolphin, KonqOperations, some filemanagement konqueror modules.
   konq_historyentry.cpp
   konq_historyloader.cpp
   konq_historyprovider.cpp
   kversioncontrolplugin.cpp  # used by dolphin and its version control plugins (deprecated)
   kversioncontrolplugin2.cpp # used by dolphin and its version control plugins
#  konq_nameandurlinputdialog.cpp # deprecated (functionality has moved to kdelibs)
   knewmenu.cpp # deprecated (functionality has moved to kdelibs)
   konq_popupmenuinformation.cpp # deprecated (functionality has moved to kdelibs)
   konq_menuactions.cpp     # deprecated (functionality has moved to kdelibs)
   konq_fileitemcapabilities.cpp # deprecated (functionality has moved to kdelibs)
)

add_library(KF5Konq ${konq_LIB_SRCS})

generate_export_header(KF5Konq BASE_NAME libkonq EXPORT_FILE_NAME libkonq_export.h)

add_library(KF5::Konq ALIAS KF5Konq)

target_link_libraries(KF5Konq
    PUBLIC
    KF5::XmlGui
    KF5::ConfigCore
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::CoreAddons
    KF5::Service
    KF5::Parts
    KF5::KDELibs4Support
    PRIVATE
    KF5::Bookmarks
    ${ZLIB_LIBRARY}
)

# For crc32 in konq_historyloader.cpp
target_include_directories(KF5Konq INTERFACE "$<INSTALL_INTERFACE:${ZLIB_INCLUDE_DIR}>")


set_target_properties(KF5Konq PROPERTIES
    VERSION ${KONQ_VERSION_STRING}
    SOVERSION ${KONQ_SOVERSION}
    EXPORT_NAME Konq
)

install(TARGETS KF5Konq EXPORT KF5KonqTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

########### konq_sound ###############

# TODO: bring back
# set(konq_sound_LIB_SRCS konq_sound.cc)
# add_library(konq_sound MODULE ${konq_sound_LIB_SRCS})
# target_link_libraries(konq_sound  ${KDE4_PHONON_LIBS})
# install(TARGETS konq_sound  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install(FILES directory_bookmarkbar.desktop DESTINATION ${KF5_DATA_INSTALL_DIR}/kbookmark)
install(FILES
    # konq_copytomenu.h - anyone needs it?
    knewmenu.h
    konq_dndpopupmenuplugin.h
    konq_events.h
    konq_fileitemcapabilities.h
    konq_historyentry.h
    konq_historyprovider.h
    konq_menuactions.h
    konq_operations.h
    konq_popupmenu.h
    konq_popupmenuinformation.h
    konq_popupmenuplugin.h
    konqmimedata.h
    kversioncontrolplugin.h
    kversioncontrolplugin2.h
    ${LibKonq_BINARY_DIR}/src/libkonq_export.h

    DESTINATION ${KF5_INCLUDE_INSTALL_DIR}
    COMPONENT Devel
)

install(FILES konqpopupmenuplugin.desktop konqdndpopupmenuplugin.desktop
    DESTINATION ${SERVICETYPES_INSTALL_DIR}
)

add_subdirectory(pics)
add_subdirectory(favicons)
add_subdirectory(Templates)
