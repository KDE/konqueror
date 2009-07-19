project(kdelauncher)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

include_directories(${KDE4_INCLUDES} ${CMAKE_SOURCE_DIR}/kdenetwork ${CMAKE_CURRENT_SOURCE_DIR}/../kdewebkit)
link_directories(${KDE4_LIB_DIR} ${CMAKE_SOURCE_DIR}/kdenetwork ${CMAKE_CURRENT_SOURCE_DIR}/../kdewebkit)
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

if(${KDE_VERSION_MINOR} EQUAL "2" AND ${KDE_VERSION_RELEASE} GREATER "69")
else(${KDE_VERSION_MINOR} EQUAL "2" AND ${KDE_VERSION_RELEASE} GREATER "69")
    # QtWebKit uses QtNetwork, to KdeWebKit uses....
    # Starting from kdelibs 4.2.70, KIO does contain the network part.
    find_package(KdeNetwork REQUIRED)
    include_directories(${KDENETWORK_INCLUDE_DIR})
endif(${KDE_VERSION_MINOR} EQUAL "2" AND ${KDE_VERSION_RELEASE} GREATER "69")

#qt4_add_resources( kdelauncher_SRCS kdelauncher.qrc )

kde4_add_executable(kdelauncher main.cpp )
target_link_libraries( kdelauncher ${KDE4_KDEUI_LIBS} kdewebkit )

if(${KDE_VERSION_MINOR} EQUAL "2" AND ${KDE_VERSION_RELEASE} GREATER "69")
else(${KDE_VERSION_MINOR} EQUAL "2" AND ${KDE_VERSION_RELEASE} GREATER "69")
    target_link_libraries(kdelauncher ${KDENETWORK_LIBRARIES})
endif(${KDE_VERSION_MINOR} EQUAL "2" AND ${KDE_VERSION_RELEASE} GREATER "69")
