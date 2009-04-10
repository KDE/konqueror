project(kdewebkit)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

# QtWebKit uses QtNetwork, to KdeWebKit uses....
find_package(KdeNetwork REQUIRED)

include_directories(${KDE4_INCLUDES} ${KDENETWORK_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

add_subdirectory(includes)
add_subdirectory(cmake)

set(kdewebkit_LIB_SRCS
    kwebview.cpp
    kwebpage.cpp
    kwebpluginfactory.cpp
    searchbar_p.cpp
    settings/webkitsettings.cpp
    settings/khtml_filter.cpp
)

kde4_add_ui_files(kdewebkit_LIB_SRCS
    searchbar_p.ui
)

kde4_add_library(kdewebkit SHARED ${kdewebkit_LIB_SRCS})

target_link_libraries(kdewebkit
    ${KDENETWORK_LIBRARIES}
    ${KDE4_KPARTS_LIBS}
    ${QT_QTWEBKIT_LIBRARY}
    ${QT_QTUITOOLS_LIBRARY}
    ${QT_QTXML_LIBRARY}
)

#set_target_properties(kdewebkit PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
set_target_properties(kdewebkit PROPERTIES VERSION 1.0.0 SOVERSION 1) # for the moment keep something low...

install(TARGETS kdewebkit ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    kwebpage.h
    kwebview.h
    kwebpluginfactory.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/kdewebkit/ COMPONENT Devel)
