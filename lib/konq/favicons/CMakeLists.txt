set(kded_favicons_SRCS favicons.cpp)

# FIXME
remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)

qt5_add_dbus_adaptor(kded_favicons_SRCS org.kde.FavIcon.xml favicons.h FavIconsModule favicons_adaptor FavIconsAdaptor)

add_library(kded_favicons MODULE ${kded_favicons_SRCS})

target_link_libraries(kded_favicons
    KF5::KIOWidgets
    KF5::DBusAddons
    KF5::KDELibs4Support
)

install(TARGETS kded_favicons DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES favicons.desktop DESTINATION  ${SERVICES_INSTALL_DIR}/kded)

install(FILES org.kde.FavIcon.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
