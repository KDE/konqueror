project(kdelauncher)

include_directories(${KDE4_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

kde4_add_executable(kdelauncher main.cpp )
target_link_libraries(kdelauncher
    ${KDE4_KDEWEBKIT_LIBS}
)
