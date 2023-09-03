
add_definitions(-DTRANSLATION_DOMAIN=\"kcmbookmarks\")

########### next target ###############

set(kcm_bookmarks_PART_SRCS
    bookmarks.cpp)

ki18n_wrap_ui(kcm_bookmarks_PART_SRCS
    bookmarks.ui)

kcoreaddons_add_plugin(kcm_bookmarks SOURCES ${kcm_bookmarks_PART_SRCS} INSTALL_NAMESPACE konqueror_kcms)
target_link_libraries(kcm_bookmarks KF${KF_MAJOR_VERSION}::KCMUtils KF${KF_MAJOR_VERSION}::KIOCore KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::GuiAddons)

kcmutils_generate_desktop_file(kcm_bookmarks)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kcm_bookmarks.desktop DESTINATION ${KDE_INSTALL_APPDIR} RENAME bookmarks.desktop)
