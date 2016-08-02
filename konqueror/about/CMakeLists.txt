find_package(KF5 REQUIRED COMPONENTS KHtml)

add_library(konq_aboutpage MODULE konq_aboutpage.cc)

target_link_libraries(konq_aboutpage KF5::KHtml KF5::KDELibs4Support Qt5::Widgets)

install(TARGETS konq_aboutpage DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES
    box-centre-konqueror.png
    top-left-konqueror.png
    lightning.png
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
