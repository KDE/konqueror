add_definitions(-DTRANSLATION_DOMAIN=\"fsview\")

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

########### next target ###############

set(libfsview_SRCS
    treemap.cpp
    fsview.cpp
    scan.cpp
    inode.cpp
    )

ecm_qt_declare_logging_category(libfsview_SRCS
  HEADER fsviewdebug.h
  IDENTIFIER FSVIEWLOG
  CATEGORY_NAME org.kde.fsview
  EXPORT fsview
  DESCRIPTION "Konqueror FileSizeView plugin")

########### next target ###############

set(fsview_SRCS main.cpp ${libfsview_SRCS} )

add_executable(fsview ${fsview_SRCS})

target_link_libraries(fsview KF5::KIOWidgets KF5::I18n KF5::IconThemes)

install(TARGETS fsview ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


########### next target ###############

set(fsviewpart_PART_SRCS fsview_part.cpp ${libfsview_SRCS})

add_library(fsviewpart MODULE ${fsviewpart_PART_SRCS})

target_link_libraries(fsviewpart KF5::Parts KF5::IconThemes)

install(TARGETS fsviewpart DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES fsview_part.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES fsview_part.rc  DESTINATION  ${KDE_INSTALL_KXMLGUI5DIR}/fsview )

ecm_qt_install_logging_categories(EXPORT fsview DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

ecm_install_icons( ICONS 22-apps-fsview.png  32-apps-fsview.png DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor   )

