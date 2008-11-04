
add_subdirectory( uasproviders )



########### next target ###############

set(kcm_kio_PART_SRCS
    policydlg.cpp
    kcookiesmain.cpp
    kcookiespolicies.cpp
    main.cpp
    smbrodlg.cpp
    useragentdlg.cpp
    kproxydlgbase.cpp
    kmanualproxydlg.cpp
    kenvvarproxydlg.cpp
    kproxydlg.cpp
    kcookiesmanagement.cpp
    useragentinfo.cpp
    useragentselectordlg.cpp
    netpref.cpp
    cache.cpp
    bookmarks.cpp
    ksaveioconfig.cpp)


kde4_add_ui_files(kcm_kio_PART_SRCS bookmarks.ui cache.ui manualproxy.ui envvarproxy.ui policydlg.ui useragentselectordlg.ui kcookiesmanagementdlg.ui useragentdlg.ui kproxydlg.ui kcookiespoliciesdlg.ui kcookiesmanagementdlg.ui)

kde4_add_plugin(kcm_kio ${kcm_kio_PART_SRCS})


target_link_libraries(kcm_kio  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KUTILS_LIBS} )

install(TARGETS kcm_kio  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES smb.desktop cookies.desktop useragent.desktop bookmarks.desktop cache.desktop lanbrowser.desktop
    netpref.desktop proxy.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES uasprovider.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
