find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Wallet)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Notifications )
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Sonnet)

find_package(Hunspell)

add_definitions(-DTRANSLATION_DOMAIN=\"webenginepart\")

if(BUILD_TESTING)
  add_definitions(-DBUILD_TESTING)
endif(BUILD_TESTING)

#Before Qt 5.15.5 QtWebEngineView didn't allow opening a remote URL by dropping it
#on the view itself, so WebEngineView needed to handle that itself.
if(Qt5WebEngineWidgets_VERSION VERSION_LESS "5.15.5")
  add_definitions(-DREMOTE_DND_NOT_HANDLED_BY_WEBENGINE)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR})

set(kwebenginepartlib_LIB_SRCS
    webenginepart.cpp
    webenginepart_ext.cpp
    webengineview.cpp
    webenginepage.cpp
    websslinfo.cpp
    webhistoryinterface.cpp
    webenginepartdownloadmanager.cpp
    webenginewallet.cpp
    webengineparterrorschemehandler.cpp
    webenginepartkiohandler.cpp
    webenginepartcookiejar.cpp
    settings/webenginesettings.cpp
    settings/webengine_filter.cpp
    ui/searchbar.cpp
    ui/passwordbar.cpp
    ui/featurepermissionbar.cpp
    about/konq_aboutpage.cpp
    webenginecustomizecacheablefieldsdlg.cpp
    webfieldsdataview.cpp
    ui/credentialsdetailswidget.cpp
    webengineurlrequestinterceptor.cpp
    spellcheckermanager.cpp
    webenginepartcontrols.cpp
    webenginepartcertificateerrordlg.cpp
    certificateerrordialogmanager.cpp
    navigationrecorder.cpp
    choosepagesaveformatdlg.cpp
)

ki18n_wrap_ui(kwebenginepartlib_LIB_SRCS webenginecustomizecacheablefieldsdlg.ui ui/credentialsdetailswidget.ui webenginepartcertificateerrordlg.ui choosepagesaveformatdlg.ui)

qt5_add_resources(kwebenginepartlib_LIB_SRCS webenginepart.qrc)

qt5_wrap_ui(kwebenginepartlib_LIB_SRCS
                  ui/searchbar.ui
)
ecm_qt_declare_logging_category(kwebenginepartlib_LIB_SRCS
    HEADER webenginepart_debug.h
    IDENTIFIER WEBENGINEPART_LOG
    CATEGORY_NAME org.kde.webenginepart
)
add_library(kwebenginepartlib ${kwebenginepartlib_LIB_SRCS})

generate_export_header(kwebenginepartlib)

target_link_libraries(kwebenginepartlib
    PUBLIC
        Qt5::Core
        Qt5::DBus
        Qt5::Gui
        Qt5::Widgets
        Qt5::WebEngineWidgets
        KF5::Parts
        KF5::Wallet
        KF5::Notifications
        KF5::KIOWidgets
    PRIVATE
        KF5::Konq
        KF5::I18n
        Qt5::PrintSupport
        KF5::SonnetCore
        KF5::IconThemes #for KIconLoader used by WebEnginePartErrorSchemeHandler
        KF5::WindowSystem  # for KUserTimestamp
        KF5::SonnetUi
)

target_include_directories(kwebenginepartlib PUBLIC
   "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>"
)
set_target_properties(kwebenginepartlib PROPERTIES OUTPUT_NAME kwebenginepart)

install(TARGETS kwebenginepartlib ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


add_library(webenginepart MODULE webenginepartfactory.cpp)
kcoreaddons_desktop_to_json(webenginepart webenginepart.desktop)


target_link_libraries(webenginepart kwebenginepartlib)

install(TARGETS webenginepart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/parts)

install(FILES webenginepart.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(FILES webenginepart.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/webenginepart)

install(FILES error.html DESTINATION ${KDE_INSTALL_DATADIR}/webenginepart)

install(FILES settings/kconf_update/webenginepart.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})

set(USE_SYSTEM_DICTIONARIES 0 CACHE BOOL "Don't build QtWebEngine dictionaries but use the system ones")
if((NOT USE_SYSTEM_DICTIONARIES) AND Hunspell_FOUND)
  set(WEBENGINEPART_OWN_DICTIONARY_DIR ${CMAKE_INSTALL_PREFIX}/share/konqueror/webengine_dictionaries CACHE PATH "The directory where dictionary files built for WebEnginePart will be installed")
  add_compile_definitions(WEBENGINEPART_OWN_DICTIONARY_DIR="${WEBENGINEPART_OWN_DICTIONARY_DIR}")
  target_compile_definitions(kdeinit_konqueror PUBLIC WEBENGINEPART_OWN_DICTIONARY_DIR="${WEBENGINEPART_OWN_DICTIONARY_DIR}")

  #COMPILE HUNSPELL DICTIONARIES AS BDIC FILES
  find_package(WebEngineDictConverter)
  if (WebEngineDictConverter_FOUND)
    #Iterate on all dictionaries
    foreach(D ${Hunspell_UNIQUE_DICTIONARIES})
      get_filename_component(base_name ${D} NAME)
      #Full name of the file to create
      set(BDIC_name ${CMAKE_CURRENT_BINARY_DIR}/${base_name}.bdic)
      #Name of the target to create
      set(BDIC_target_name ${base_name}.bdic_target)
      #Check that both .dic and .aff files exist for the given dictionary, otherwise the conversion tool will fail
      if (EXISTS ${D}.dic AND EXISTS ${D}.aff)
        add_custom_command(
          OUTPUT ${BDIC_name}
          COMMAND ${WebEngineDictConverter_EXECUTABLE} ${D} ${BDIC_name}
        )
        #Install the bdic file
        install(FILES ${BDIC_name} DESTINATION ${WEBENGINEPART_OWN_DICTIONARY_DIR})
        #Create a new target which depends on the file
        add_custom_target(${BDIC_target_name} DEPENDS ${BDIC_name})
        #Add the new target as dependency to kwebenginepartlib, otherwise the dictionaries won't be built
        add_dependencies(kwebenginepartlib ${BDIC_target_name})
      endif()
    endforeach()
  endif()
endif()


add_subdirectory(about)
