
add_definitions(-DTRANSLATION_DOMAIN=\"kcmkio\")

add_subdirectory( uasproviders )

########### next target ###############

set(kcm_kio_PART_SRCS
    main.cpp
    kcookiesmain.cpp
    kcookiespolicies.cpp
    kcookiesmanagement.cpp
    kcookiespolicyselectiondlg.cpp
    smbrodlg.cpp
    useragentdlg.cpp
    kproxydlg.cpp
    useragentinfo.cpp
    useragentselectordlg.cpp
    netpref.cpp
    cache.cpp
    bookmarks.cpp
    ksaveioconfig.cpp)

ki18n_wrap_ui(kcm_kio_PART_SRCS
    bookmarks.ui
    cache.ui
    useragentselectordlg.ui
    useragentdlg.ui
    kproxydlg.ui
    kcookiespolicies.ui
    kcookiesmanagement.ui
    kcookiespolicyselectiondlg.ui)

add_library(kcm_kio MODULE ${kcm_kio_PART_SRCS})

target_link_libraries(kcm_kio KF5::KCMUtils KF5::KIOCore KF5::I18n KF5::KDELibs4Support)

install(TARGETS kcm_kio  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES smb.desktop cookies.desktop useragent.desktop bookmarks.desktop cache.desktop
    netpref.desktop proxy.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES uasprovider.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
