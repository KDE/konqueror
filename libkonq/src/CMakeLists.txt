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
)

ecm_qt_declare_logging_category(konq_LIB_SRCS HEADER libkonq_debug.h IDENTIFIER LIBKONQ_LOG CATEGORY_NAME org.kde.libkonq)

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
    Qt5::WebEngineWidgets
    PRIVATE
    KF5::Bookmarks
    KF5::I18n
    ${ZLIB_LIBRARY}
)

# For crc32 in konq_historyloader.cpp
target_include_directories(KF5Konq PRIVATE ${ZLIB_INCLUDE_DIR})


set_target_properties(KF5Konq PROPERTIES
    VERSION ${KONQ_VERSION}
    SOVERSION ${KONQ_SOVERSION}
    EXPORT_NAME Konq
)

install(TARGETS KF5Konq EXPORT KF5KonqTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(FILES directory_bookmarkbar.desktop DESTINATION ${KDE_INSTALL_DATADIR_KF5}/kbookmark)
install(FILES
    konq_events.h
    konq_historyentry.h
    konq_historyprovider.h
    konq_popupmenu.h
    konq_kpart_plugin.h
    ${LibKonq_BINARY_DIR}/src/libkonq_export.h
    asyncselectorinterface.h

    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}
    COMPONENT Devel
)

