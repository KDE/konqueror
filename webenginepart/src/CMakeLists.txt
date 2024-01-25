find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS Wallet)
find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS Notifications )
find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS Sonnet)

find_package(Hunspell)

add_definitions(-DTRANSLATION_DOMAIN=\"webenginepart\")

if(BUILD_TESTING)
  add_definitions(-DBUILD_TESTING)
endif(BUILD_TESTING)

#Before Qt 5.15.5 QtWebEngineView didn't allow opening a remote URL by dropping it
#on the view itself, so WebEngineView needed to handle that itself.
if(KF_MAJOR_VERSION STRLESS "6" AND QtWebEngineWidgets_VERSION VERSION_LESS "5.15.5")
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
    qtwebengine6compat.h
    schemehandlers/execschemehandler.cpp
    schemehandlers/errorschemehandler.cpp
    schemehandlers/kiohandler.cpp
)

if (DontUseKCookieJar)
  list(APPEND kwebenginepartlib_LIB_SRCS
    cookies/webenginepartcookiejar6.cpp
    cookies/cookiealertdlg.cpp
    )
  ki18n_wrap_ui(kwebenginepartlib_LIB_SRCS cookies/cookiealertdlg.ui)
else()
  list(APPEND kwebenginepartlib_LIB_SRCS cookies/webenginepartcookiejar_kio.cpp)
endif()

ki18n_wrap_ui(kwebenginepartlib_LIB_SRCS
  webenginecustomizecacheablefieldsdlg.ui
  ui/credentialsdetailswidget.ui
  webenginepartcertificateerrordlg.ui
  choosepagesaveformatdlg.ui
  )

if (QT_MAJOR_VERSION STREQUAL 6)
list(APPEND kwebenginepartlib_LIB_SRCS profile.cpp)
endif()

qt_add_resources(kwebenginepartlib_LIB_SRCS webenginepart.qrc)

qt_wrap_ui(kwebenginepartlib_LIB_SRCS
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
        Qt${KF_MAJOR_VERSION}::Core
        Qt${KF_MAJOR_VERSION}::DBus
        Qt${KF_MAJOR_VERSION}::Gui
        Qt${KF_MAJOR_VERSION}::Widgets
        Qt${KF_MAJOR_VERSION}::WebEngineWidgets
        KF${KF_MAJOR_VERSION}::Parts
        KF${KF_MAJOR_VERSION}::Wallet
        KF${KF_MAJOR_VERSION}::Notifications
        KF${KF_MAJOR_VERSION}::KIOWidgets
        KF${KF_MAJOR_VERSION}::TextWidgets
        KF${KF_MAJOR_VERSION}::Konq
    PRIVATE
        KF${KF_MAJOR_VERSION}::I18n
        Qt${KF_MAJOR_VERSION}::PrintSupport
        KF${KF_MAJOR_VERSION}::SonnetCore
        KF${KF_MAJOR_VERSION}::IconThemes #for KIconLoader used by WebEnginePartErrorSchemeHandler
        KF${KF_MAJOR_VERSION}::WindowSystem  # for KUserTimestamp
        KF${KF_MAJOR_VERSION}::SonnetUi
)

target_include_directories(kwebenginepartlib PUBLIC
   "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>"
)
set_target_properties(kwebenginepartlib PROPERTIES OUTPUT_NAME kwebenginepart)

install(TARGETS kwebenginepartlib ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


add_library(webenginepart MODULE webenginepartfactory.cpp)

target_link_libraries(webenginepart kwebenginepartlib)

install(TARGETS webenginepart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${KF_MAJOR_VERSION}/parts)

install(FILES error.html DESTINATION ${KDE_INSTALL_DATADIR}/webenginepart)

install(FILES settings/kconf_update/webenginepart.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})

set(USE_SYSTEM_DICTIONARIES 0 CACHE BOOL "Don't build QtWebEngine dictionaries but use the system ones")
if((NOT USE_SYSTEM_DICTIONARIES) AND Hunspell_FOUND)
  set(WEBENGINEPART_OWN_DICTIONARY_DIR ${CMAKE_INSTALL_PREFIX}/share/konqueror/webengine_dictionaries CACHE PATH "The directory where dictionary files built for WebEnginePart will be installed")
  add_compile_definitions(WEBENGINEPART_OWN_DICTIONARY_DIR="${WEBENGINEPART_OWN_DICTIONARY_DIR}")
  
  if (KF_MAJOR_VERSION STRLESS "6")
    set(konqueror_internal_lib_target kdeinit_konqueror)
  else()
    set(konqueror_internal_lib_target konqueror_internal_lib)
  endif()
  target_compile_definitions(${konqueror_internal_lib_target} PUBLIC WEBENGINEPART_OWN_DICTIONARY_DIR="${WEBENGINEPART_OWN_DICTIONARY_DIR}")

  find_package(WebEngineDictConverter)
  #COMPILE HUNSPELL DICTIONARIES AS BDIC FILES
  #Qt6 provides a command which automatically builds dictionaries (qt_add_webengine_dictionary, see https://doc.qt.io/qt-6/qt-add-webengine-dictionary.html),
  #but I can't find out how to install the dictionaries. For the time being, continue relying on custom code
  #TODO KF6: see how to install dictionaries created using qt_add_webengine_dictionary and use it
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
