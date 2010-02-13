project(kdelauncher)

include_directories(${KDE4_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

kde4_add_executable(kdelauncher main.cpp )
target_link_libraries(kdelauncher
    ${QT_QTWEBKIT_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_KIO_LIBRARY}
    ${KDE4_KDEWEBKIT_LIBS}
)
