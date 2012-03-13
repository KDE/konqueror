include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/css
    )


set(kcm_konqhtml_PART_SRCS 
   generalopts.cpp
   htmlopts.cpp 
   jsopts.cpp 
   javaopts.cpp 
   pluginopts.cpp 
   appearance.cpp 
   khttpoptdlg.cpp 
   policydlg.cpp 
   main.cpp 
   jspolicies.cpp 
   policies.cpp 
   domainlistview.cpp 
   filteropts.cpp
   css/template.cpp
   css/kcmcss.cpp
   )

kde4_add_ui_files(kcm_konqhtml_PART_SRCS
    nsconfigwidget.ui
    advancedTabOptions.ui
    css/csscustom.ui
    css/cssconfig.ui
    )

kde4_add_plugin(kcm_konqhtml ${kcm_konqhtml_PART_SRCS})


target_link_libraries(kcm_konqhtml ${KDE4_KPARTS_LIBS})

install(TARGETS kcm_konqhtml  DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES khtml_general.desktop khtml_behavior.desktop khtml_java_js.desktop khtml_appearance.desktop khtml_filter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES css/template.css  DESTINATION  ${DATA_INSTALL_DIR}/kcmcss )
