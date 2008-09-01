project(kdenetwork)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

include_directories(${KDE4_INCLUDES})
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

add_subdirectory(includes)

set(kdenetwork_LIB_SRCS
    knetworkreply.cpp
    knetworkaccessmanager.cpp
)

kde4_add_library(kdenetwork SHARED ${kdenetwork_LIB_SRCS})

target_link_libraries(kdenetwork
    ${KDE4_KIO_LIBS}
    ${QT_QTNETWORK_LIBRARY}
)

#set_target_properties(kdenetwork PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
set_target_properties(kdenetwork PROPERTIES VERSION 1.0.0 SOVERSION 1) # for the moment keep something low...

install(TARGETS kdenetwork ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    knetworkaccessmanager.h
    knetworkreply.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/kdenetwork/ COMPONENT Devel)
