project(kdelauncher)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

include_directories(${KDE4_INCLUDES} ${CMAKE_SOURCE_DIR}/kdenetwork ${CMAKE_CURRENT_SOURCE_DIR}/../kdewebkit)
link_directories(${KDE4_LIB_DIR} ${CMAKE_SOURCE_DIR}/kdenetwork ${CMAKE_CURRENT_SOURCE_DIR}/../kdewebkit)
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

#qt4_add_resources( kdelauncher_SRCS kdelauncher.qrc )

kde4_add_executable(kdelauncher main.cpp )
target_link_libraries( kdelauncher ${KDE4_KDEUI_LIBS} kdewebkit )
