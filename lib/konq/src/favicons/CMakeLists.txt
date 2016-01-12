set(kded_favicons_SRCS favicons.cpp)

# FIXME
remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)

qt5_add_dbus_adaptor(kded_favicons_SRCS org.kde.FavIcon.xml favicons.h FavIconsModule favicons_adaptor FavIconsAdaptor)

kcoreaddons_add_plugin(kded_favicons INSTALL_NAMESPACE "kf5/kded" JSON favicons.json SOURCES ${kded_favicons_SRCS})
set_target_properties(kded_favicons PROPERTIES
    OUTPUT_NAME favicons
)


target_link_libraries(kded_favicons
    Qt5::Gui
    KF5::KIOCore
    KF5::I18n
    KF5::DBusAddons
)

install(FILES org.kde.FavIcon.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
