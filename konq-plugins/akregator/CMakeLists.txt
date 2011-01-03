########### next target ###############

MESSAGE(STATUS "akregator popupmenu plugin disabled. Doable via servicemenu or should we re-introduce ServiceMenusPlugin?")

#set(akregatorkonqplugin_PART_SRCS akregatorplugin.cpp pluginbase.cpp )
#
#kde4_add_plugin(akregatorkonqplugin ${akregatorkonqplugin_PART_SRCS})
#
#target_link_libraries(akregatorkonqplugin  ${KDE4_KDE3SUPPORT_LIBS} ${QT_QTXML_LIBRARY} konq
#    ${KDE4_KHTML_LIBS} )
#
#install(TARGETS akregatorkonqplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(akregatorkonqfeedicon_PART_SRCS konqfeedicon.cpp feeddetector.cpp pluginbase.cpp )

kde4_add_plugin(akregatorkonqfeedicon ${akregatorkonqfeedicon_PART_SRCS})

target_link_libraries(akregatorkonqfeedicon ${KDE4_KPARTS_LIBS} )

install(TARGETS akregatorkonqfeedicon DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

#install( FILES akregator_konqplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES akregator_konqfeedicon.desktop akregator_konqfeedicon.rc  DESTINATION  ${DATA_INSTALL_DIR}/khtml/kpartplugins )
install( FILES akregator_konqfeedicon.desktop akregator_konqfeedicon.rc  DESTINATION  ${DATA_INSTALL_DIR}/kwebkitpart/kpartplugins )

install( FILES feed.png DESTINATION ${DATA_INSTALL_DIR}/akregator/pics )
