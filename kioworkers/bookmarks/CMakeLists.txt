add_definitions(-DTRANSLATION_DOMAIN=\"kio6_bookmarks\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_library(kio_bookmarks MODULE)
set_target_properties(kio_bookmarks PROPERTIES
    OUTPUT_NAME "bookmarks"
)

target_sources(kio_bookmarks PRIVATE
    kio_bookmarks.cpp
    kio_bookmarks_html.cpp
    kio_bookmarks_pixmap.cpp
)

target_link_libraries(kio_bookmarks KF${QT_MAJOR_VERSION}::ConfigWidgets KF${QT_MAJOR_VERSION}::KIOCore KF${QT_MAJOR_VERSION}::I18n KF${QT_MAJOR_VERSION}::Bookmarks KF${QT_MAJOR_VERSION}::Solid KF${QT_MAJOR_VERSION}::GuiAddons KF${QT_MAJOR_VERSION}::KIOFileWidgets)

install(TARGETS kio_bookmarks DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/kio)

install( FILES kio_bookmarks.css DESTINATION  ${KDE_INSTALL_DATADIR}/kio_bookmarks )
