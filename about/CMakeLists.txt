add_library(konq_aboutpage MODULE konq_aboutpage.cpp)

target_link_libraries(konq_aboutpage
   KF5::KHtml
   KF5::IconThemes
   Qt5::Widgets
)

install(TARGETS konq_aboutpage DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES
    top-left-konqueror.png
    launch.html
    intro.html
    specs.html
    tips.html
    plugins.html
    plugins_rtl.html
    konq.css
    DESTINATION ${KDE_INSTALL_DATADIR}/konqueror/about)
install(FILES konqaboutpage.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
install(FILES konq_aboutpage.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
