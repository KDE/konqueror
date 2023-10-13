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

if (DontUseKCookieJar)
    list(APPEND kcm_konqhtml_PART_SRCS
        cookies/kcookiesmain.cpp
        cookies/kcookiesmanagement.cpp
        cookies/kcookiespolicies.cpp
        cookies/kcookiespolicyselectiondlg.cpp
        )
    ki18n_wrap_ui(kcm_konqhtml_PART_SRCS
        cookies/kcookiesmanagement.ui
        cookies/kcookiespolicies.ui
        cookies/kcookiespolicyselectiondlg.ui
        )
endif()

ki18n_wrap_ui(kcm_konqhtml_PART_SRCS
    advancedTabOptions.ui
    css/csscustom.ui
    css/cssconfig.ui
    cache/cache.ui
    useragent/useragent.ui
    )

add_library(kcm_konqhtml_static STATIC ${kcm_konqhtml_PART_SRCS})
set_target_properties(kcm_konqhtml_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(kcm_konqhtml_static
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::ConfigWidgets
    KF${KF_MAJOR_VERSION}::KCMUtils
    KF${KF_MAJOR_VERSION}::TextWidgets
    KF${KF_MAJOR_VERSION}::Parts
    KF${KF_MAJOR_VERSION}::ItemViews
    KF${KF_MAJOR_VERSION}::Codecs
    Qt${KF_MAJOR_VERSION}::WebEngineWidgets
    KF${KF_MAJOR_VERSION}::Konq)

kcoreaddons_add_plugin(khtml_general SOURCES plugins/khtml_general.cpp INSTALL_NAMESPACE konqueror_kcms)
target_link_libraries(khtml_general kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_behavior SOURCES plugins/khtml_behavior.cpp INSTALL_NAMESPACE konqueror_kcms)
target_link_libraries(khtml_behavior kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_java_js SOURCES plugins/khtml_java_js.cpp INSTALL_NAMESPACE konqueror_kcms)
target_link_libraries(khtml_java_js kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_appearance SOURCES plugins/khtml_appearance.cpp INSTALL_NAMESPACE konqueror_kcms)
target_link_libraries(khtml_appearance kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_filter SOURCES plugins/khtml_filter.cpp INSTALL_NAMESPACE konqueror_kcms)
target_link_libraries(khtml_filter kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_cache SOURCES plugins/khtml_cache.cpp INSTALL_NAMESPACE konqueror_kcms)
target_link_libraries(khtml_cache kcm_konqhtml_static)

kcoreaddons_add_plugin(khtml_useragent SOURCES plugins/khtml_useragent.cpp INSTALL_NAMESPACE konqueror_kcms)
target_link_libraries(khtml_useragent kcm_konqhtml_static)

if (DontUseKCookieJar)
    kcoreaddons_add_plugin(khtml_cookies SOURCES plugins/khtml_cookies.cpp INSTALL_NAMESPACE konqueror_kcms)
    target_link_libraries(khtml_cookies kcm_konqhtml_static)
endif()

install( FILES css/template.css  DESTINATION  ${KDE_INSTALL_DATADIR}/kcmcss )
install(FILES useragent/useragenttemplatesrc DESTINATION ${KDE_INSTALL_CONFDIR})
