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

add_library(searchbarplugin MODULE ${searchbarplugin_PART_SRCS})

target_link_libraries(searchbarplugin KF5::Parts Qt5::Script KF5::KDELibs4Support)

install(TARGETS searchbarplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### install files ###############

install( FILES searchbar.rc searchbar.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/konqueror/kpartplugins )

