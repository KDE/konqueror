project (searchbar)
add_definitions(-DTRANSLATION_DOMAIN=\"searchbarplugin\")

add_subdirectory( icons )

########### next target ###############

set(searchbarplugin_PART_SRCS
    searchbar.cpp
    SuggestionEngine.cpp
    WebShortcutWidget.cpp)
ecm_qt_declare_logging_category(searchbarplugin_PART_SRCS HEADER searchbar_debug.h IDENTIFIER SEARCHBAR_LOG CATEGORY_NAME org.kde.konqueror.searchbar)

add_library(searchbarplugin MODULE ${searchbarplugin_PART_SRCS})

target_link_libraries(searchbarplugin KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::TextWidgets KF${KF_MAJOR_VERSION}::Parts KF${KF_MAJOR_VERSION}::Konq)

install(TARGETS searchbarplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/konqueror/kpartplugins)
install(FILES searchbarplugin.rc DESTINATION ${KDE_INSTALL_DATADIR}/konqueror/partsrcfiles)
