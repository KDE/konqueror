project (searchbar)

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

kde4_add_plugin(searchbarplugin ${searchbarplugin_PART_SRCS})

target_link_libraries(searchbarplugin ${KDE4_KPARTS_LIBS} ${QT_QTSCRIPT_LIBRARY} )

install(TARGETS searchbarplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES searchbar.rc searchbar.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqueror/kpartplugins )

