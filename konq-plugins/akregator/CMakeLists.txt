########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"akregator_konqplugin\")
MESSAGE(STATUS "akregator popupmenu plugin disabled. Doable via servicemenu or should we re-introduce ServiceMenusPlugin?")

#set(akregatorkonqplugin_PART_SRCS akregatorplugin.cpp pluginbase.cpp )
#
#kde4_add_plugin(akregatorkonqplugin ${akregatorkonqplugin_PART_SRCS})
#
#target_link_libraries(akregatorkonqplugin  ${KDE4_KDE3SUPPORT_LIBS} Qt5::Xml konq
#    KF5::KHtml )
#
#install(TARGETS akregatorkonqplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### next target ###############

set(akregatorkonqfeedicon_PART_SRCS konqfeedicon.cpp feeddetector.cpp pluginbase.cpp )

add_library(akregatorkonqfeedicon MODULE ${akregatorkonqfeedicon_PART_SRCS})

target_compile_definitions(akregatorkonqfeedicon PRIVATE TRANSLATION_DOMAIN="akregator_konqplugin")

target_link_libraries(akregatorkonqfeedicon KF5::Parts KF5::KDELibs4Support)

install(TARGETS akregatorkonqfeedicon DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

#install( FILES akregator_konqplugin.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES akregator_konqfeedicon.desktop akregator_konqfeedicon.rc  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )
install( FILES akregator_konqfeedicon.desktop akregator_konqfeedicon.rc  DESTINATION  ${KDE_INSTALL_DATADIR}/kwebkitpart/kpartplugins )

install( FILES feed.png DESTINATION ${KDE_INSTALL_DATADIR}/akregator/pics )
