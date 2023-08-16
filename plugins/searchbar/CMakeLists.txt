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

target_link_libraries(searchbarplugin KF5::I18n KF5::TextWidgets KF5::Parts KF5Konq)

install(TARGETS searchbarplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/konqueror/kpartplugins)
install(FILES searchbarplugin.rc DESTINATION ${KDE_INSTALL_DATADIR}/konqueror/partsrcfiles)
