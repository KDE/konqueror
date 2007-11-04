
########### next target ###############

set(kcm_konq_PART_SRCS
   behaviour.cpp
   globalpaths.cpp
   previews.cpp
   browser.cpp
   konqkcmfactory.cpp
   #desktopbehavior_impl.cpp
   kcustommenueditor.cpp)

if(Q_WS_X11)
 #set(kdesktop_xml  ${KDEBASE_WORKSPACE_SOURCE_DIR}/kdesktop/org.kde.kdesktop.Desktop.xml)
 #QT4_ADD_DBUS_INTERFACE( kcm_konq_PART_SRCS ${kdesktop_xml} kdesktop_interface )
endif(Q_WS_X11)
#kde4_add_ui_files(kcm_konq_PART_SRCS desktopbehavior.ui )




kde4_add_plugin(kcm_konq ${kcm_konq_PART_SRCS})


target_link_libraries(kcm_konq  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KUTILS_LIBS} )

install(TARGETS kcm_konq  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############
install( FILES
   #desktopbehavior.desktop
   desktoppath.desktop
   filebehavior.desktop
   fileappearance.desktop
   filepreviews.desktop
   filebrowser.desktop 
   DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES overlapping.png onlyone.png  DESTINATION  ${DATA_INSTALL_DIR}/kcontrol/pics )

