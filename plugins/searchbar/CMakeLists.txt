project (searchbar)
add_definitions(-DTRANSLATION_DOMAIN=\"searchbarplugin\")
find_package(Qt5 REQUIRED Script)

add_subdirectory( icons )
add_subdirectory( opensearch )

########### next target ###############

set(searchbarplugin_PART_SRCS
    searchbar.cpp
    OpenSearchManager.cpp
    SuggestionEngine.cpp
    WebShortcutWidget.cpp
    opensearch/OpenSearchEngine.cpp
    opensearch/OpenSearchReader.cpp
    opensearch/OpenSearchWriter.cpp)
ecm_qt_declare_logging_category(searchbarplugin_PART_SRCS HEADER searchbar_debug.h IDENTIFIER SEARCHBAR_LOG CATEGORY_NAME org.kde.konqueror.searchbar)

add_library(searchbarplugin MODULE ${searchbarplugin_PART_SRCS})

kcoreaddons_desktop_to_json(searchbarplugin searchbar.desktop)
target_link_libraries(searchbarplugin KF5::I18n KF5::TextWidgets KF5::Parts Qt5::Script KF5Konq)

install(TARGETS searchbarplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/konqueror/kpartplugins)
install(FILES searchbarplugin.rc DESTINATION ${KDE_INSTALL_DATADIR}/konqueror/partsrcfiles)
