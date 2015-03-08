########### khtmlkttsdplugin ###############

include(MacroOptionalDependPackage)

set(khtmlkttsdplugin_PART_SRCS khtmlkttsd.cpp )

qt5_add_dbus_interfaces(khtmlkttsdplugin_PART_SRCS ${KDE4_DBUS_INTERFACES_DIR}/org.kde.KSpeech.xml)

add_library(khtmlkttsdplugin MODULE ${khtmlkttsdplugin_PART_SRCS})

target_link_libraries(khtmlkttsdplugin  KF5::Parts )

install(TARGETS khtmlkttsdplugin  DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/khtml/kpartplugins )
install( FILES khtmlkttsd.rc khtmlkttsd.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/kwebkitpart/kpartplugins )

