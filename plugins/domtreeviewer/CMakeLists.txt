find_package(KF5 REQUIRED KHtml)
add_definitions(-DTRANSLATION_DOMAIN=\"domtreeviewer\")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${KDE4_ENABLE_EXCEPTIONS}")
########### next target ###############

set(domtreeviewerplugin_PART_SRCS
   plugin_domtreeviewer.cpp
   domtreeview.cpp
   domlistviewitem.cpp
   domtreewindow.cpp
   domtreecommands.cpp
   signalreceiver.cpp )

ki18n_wrap_ui(domtreeviewerplugin_PART_SRCS domtreeviewbase.ui texteditwidget.ui elementeditwidget.ui attributeeditwidget.ui messagedialog.ui)

add_library(domtreeviewerplugin MODULE ${domtreeviewerplugin_PART_SRCS})



target_link_libraries(domtreeviewerplugin KF5::KHtml KF5::KDELibs4Support)

install(TARGETS domtreeviewerplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES plugin_domtreeviewer.rc plugin_domtreeviewer.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )
install( FILES domtreeviewerui.rc  DESTINATION  ${KDE_INSTALL_DATADIR}/domtreeviewer )





