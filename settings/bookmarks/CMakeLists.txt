
add_definitions(-DTRANSLATION_DOMAIN=\"kcmbookmarks\")

########### next target ###############

set(kcm_bookmarks_PART_SRCS
    bookmarks.cpp)

ki18n_wrap_ui(kcm_bookmarks_PART_SRCS
    bookmarks.ui)

kcoreaddons_add_plugin(kcm_bookmarks SOURCES ${kcm_bookmarks_PART_SRCS} INSTALL_NAMESPACE konqueror_kcms)
kcoreaddons_desktop_to_json(kcm_bookmarks bookmarks.desktop)
target_link_libraries(kcm_bookmarks KF5::KCMUtils KF5::KIOCore KF5::I18n KF5::GuiAddons)
