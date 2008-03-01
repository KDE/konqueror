add_subdirectory(pics)

set(kcm_useraccount_PART_SRCS
    chfnprocess.cpp
    main.cpp
    chfacedlg.cpp)

kde4_add_ui_files(kcm_useraccount_PART_SRCS
    main_widget.ui
    faceDlg.ui)

kde4_add_ui_files(kcm_useraccount_PART_SRCS main_widget.ui)

kde4_add_kcfg_files(kcm_useraccount_PART_SRCS settings.kcfgc pass.kcfgc)

kde4_add_plugin(kcm_useraccount ${kcm_useraccount_PART_SRCS})

target_link_libraries(kcm_useraccount konq ${KDE4_KDESU_LIBS})

install(TARGETS kcm_useraccount DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES kcm_useraccount.desktop DESTINATION  ${SERVICES_INSTALL_DIR})
install(FILES kcm_useraccount.kcfg kcm_useraccount_pass.kcfg DESTINATION ${KCFG_INSTALL_DIR})

kde4_install_icons(${ICON_INSTALL_DIR})
