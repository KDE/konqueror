add_definitions(-DTRANSLATION_DOMAIN=\"libkonq\")

########### libkonq ###############

set(konq_LIB_SRCS
   konq_popupmenu.cpp       # now only used by konqueror, could move there
   konq_events.cpp
   konq_historyentry.cpp
   konq_historyloader.cpp
   konq_historyprovider.cpp   # konqueror and konqueror/sidebar
   konq_spellcheckingconfigurationdispatcher.cpp #konqueror and webenginepart
   konq_kpart_plugin.cpp
   asyncselectorinterface.cpp
   browseropenorsavequestion.cpp
   historyprovider.cpp
   htmlextension.cpp
   htmlsettingsinterface.cpp
   textextension.cpp
   interfaces/browser.cpp
   interfaces/cookiejar.cpp
   interfaces/common.h
   interfaces/downloaderextension.cpp
   libkonq_utils.cpp
   browserarguments.cpp
   browserextension.cpp
   windowargs.cpp
   browserinterface.cpp
)

ecm_qt_declare_logging_category(konq_LIB_SRCS HEADER libkonq_debug.h IDENTIFIER LIBKONQ_LOG CATEGORY_NAME org.kde.libkonq)

add_library(KF${KF_MAJOR_VERSION}Konq ${konq_LIB_SRCS})

generate_export_header(KF${KF_MAJOR_VERSION}Konq BASE_NAME libkonq EXPORT_FILE_NAME libkonq_export.h)

add_library(KF${KF_MAJOR_VERSION}::Konq ALIAS KF${KF_MAJOR_VERSION}Konq)

target_link_libraries(KF${KF_MAJOR_VERSION}Konq
    PUBLIC
    KF${KF_MAJOR_VERSION}::XmlGui
    KF${KF_MAJOR_VERSION}::ConfigCore
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::KIOFileWidgets
    KF${KF_MAJOR_VERSION}::CoreAddons
    KF${KF_MAJOR_VERSION}::Service
    KF${KF_MAJOR_VERSION}::Parts
    Qt${KF_MAJOR_VERSION}::WebEngineWidgets
    KF${KF_MAJOR_VERSION}::TextWidgets
    PRIVATE
    KF${KF_MAJOR_VERSION}::Bookmarks
    KF${KF_MAJOR_VERSION}::I18n
    ${ZLIB_LIBRARY}
)

target_link_libraries(KF${KF_MAJOR_VERSION}Konq PRIVATE KF${KF_MAJOR_VERSION}::BookmarksWidgets)

# For crc32 in konq_historyloader.cpp
target_include_directories(KF${KF_MAJOR_VERSION}Konq PRIVATE ${ZLIB_INCLUDE_DIR})


set_target_properties(KF${KF_MAJOR_VERSION}Konq PROPERTIES
    VERSION ${KONQ_VERSION}
    SOVERSION ${KONQ_SOVERSION}
    EXPORT_NAME Konq
)

install(TARGETS KF${KF_MAJOR_VERSION}Konq EXPORT KF${KF_MAJOR_VERSION}KonqTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(FILES directory_bookmarkbar.desktop DESTINATION ${KDE_INSTALL_DATADIR_KF}/kbookmark)
install(FILES
    konq_events.h
    konq_historyentry.h
    konq_historyprovider.h
    konq_popupmenu.h
    konq_kpart_plugin.h
    ${LibKonq_BINARY_DIR}/src/libkonq_export.h
    asyncselectorinterface.h

    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}
    COMPONENT Devel
)

