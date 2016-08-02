########### konqsidebar_web module ###############

set(konqsidebar_web_PART_SRCS web_module.cpp )

add_library(konqsidebar_web MODULE ${konqsidebar_web_PART_SRCS})

target_link_libraries(konqsidebar_web
   KF5::KHtml
   konqsidebarplugin
   KF5::Konq
   KF5::KIOGui)

install(TARGETS konqsidebar_web  DESTINATION ${KDE_INSTALL_PLUGINDIR} )

########### install files ###############

install( FILES konqsidebar_web.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/konqsidebartng/plugins )
