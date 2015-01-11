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

add_library(searchbarplugin MODULE ${searchbarplugin_PART_SRCS})

target_link_libraries(searchbarplugin KF5::Parts Qt5::Script )

install(TARGETS searchbarplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES searchbar.rc searchbar.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqueror/kpartplugins )

