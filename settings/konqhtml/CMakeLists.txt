add_definitions(-DTRANSLATION_DOMAIN=\"kcmkonqhtml\")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/css
    )


set(kcm_konqhtml_PART_SRCS 
   generalopts.cpp
   htmlopts.cpp 
   jsopts.cpp 
   javaopts.cpp 
   appearance.cpp 
   policydlg.cpp 
   main.cpp 
   jspolicies.cpp 
   policies.cpp 
   domainlistview.cpp 
   filteropts.cpp
   css/template.cpp
   css/kcmcss.cpp
   cache/cache.cpp
   useragent/useragent.cpp
   )

ki18n_wrap_ui(kcm_konqhtml_PART_SRCS
    advancedTabOptions.ui
    css/csscustom.ui
    css/cssconfig.ui
    cache/cache.ui
    useragent/useragent.ui
    )

add_library(kcm_konqhtml_static STATIC ${kcm_konqhtml_PART_SRCS})
target_link_libraries(kcm_konqhtml_static KF5::I18n KF5::ConfigWidgets KF5::TextWidgets KF5::Parts KF5::ItemViews Qt5::WebEngineWidgets)

kcoreaddons_add_plugin(khtml_general SOURCES plugins/khtml_general.cpp INSTALL_NAMESPACE konqueror_kcms)
kcoreaddons_desktop_to_json(khtml_general khtml_general.desktop)
target_link_libraries(khtml_general kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_behavior SOURCES plugins/khtml_behavior.cpp INSTALL_NAMESPACE konqueror_kcms)
kcoreaddons_desktop_to_json(khtml_behavior khtml_behavior.desktop)
target_link_libraries(khtml_behavior kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_java_js SOURCES plugins/khtml_java_js.cpp INSTALL_NAMESPACE konqueror_kcms)
kcoreaddons_desktop_to_json(khtml_java_js khtml_java_js.desktop)
target_link_libraries(khtml_java_js kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_appearance SOURCES plugins/khtml_appearance.cpp INSTALL_NAMESPACE konqueror_kcms)
kcoreaddons_desktop_to_json(khtml_appearance khtml_appearance.desktop)
target_link_libraries(khtml_appearance kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_filter SOURCES plugins/khtml_filter.cpp INSTALL_NAMESPACE konqueror_kcms)
kcoreaddons_desktop_to_json(khtml_filter khtml_filter.desktop)
target_link_libraries(khtml_filter kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_cache SOURCES plugins/khtml_cache.cpp INSTALL_NAMESPACE konqueror_kcms)
kcoreaddons_desktop_to_json(khtml_cache cache/khtml_cache.desktop)
target_link_libraries(khtml_cache kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_useragent SOURCES plugins/khtml_useragent.cpp INSTALL_NAMESPACE konqueror_kcms)
kcoreaddons_desktop_to_json(khtml_useragent useragent/khtml_useragent.desktop)
target_link_libraries(khtml_useragent kcm_konqhtml_static)

install( FILES css/template.css  DESTINATION  ${KDE_INSTALL_DATADIR}/kcmcss )
install(FILES useragent/useragenttemplatesrc DESTINATION ${KDE_INSTALL_CONFDIR})
