project(kdewebkit)

set(kdewebkit_LIB_SRCS
    kwebview.cpp
    kwebpage.cpp
    kwebpluginfactory.cpp
    network/knetworkreply.cpp
    network/knetworkaccessmanager.cpp
    searchbar.cpp
    settings/webkitsettings.cpp
    webkitglobal.cpp
)

kde4_add_ui_files(kdewebkit_LIB_SRCS
    searchbar.ui
)

kde4_add_library(kdewebkit SHARED ${kdewebkit_LIB_SRCS})

target_link_libraries(kdewebkit
    ${KDE4_KPARTS_LIBS}
    ${QT_QTWEBKIT_LIBRARY}
    ${QT_QTUITOOLS_LIBRARY}
)

#set_target_properties(kdewebkit PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
set_target_properties(kdewebkit PROPERTIES VERSION 1.0.0 SOVERSION 1) # for the moment keep something low...

install(TARGETS kdewebkit ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    kwebpage.h
    kwebview.h
    kwebpluginfactory.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/webkitkde/ COMPONENT Devel)
