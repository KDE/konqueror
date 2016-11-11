
add_definitions(-DTRANSLATION_DOMAIN=\"kcmbookmarks\")

########### next target ###############

set(kcm_bookmarks_PART_SRCS
    main.cpp
    bookmarks.cpp)

ki18n_wrap_ui(kcm_bookmarks_PART_SRCS
    bookmarks.ui)

add_library(kcm_bookmarks MODULE ${kcm_bookmarks_PART_SRCS})

target_link_libraries(kcm_bookmarks KF5::KCMUtils KF5::KIOCore KF5::I18n KF5::GuiAddons)

install(TARGETS kcm_bookmarks  DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### install files ###############

install( FILES bookmarks.desktop DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )

