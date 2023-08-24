
find_package(QtTextToSpeech ${QT_REQUIRED_VERSION} QUIET)
set_package_properties(QtTextToSpeech PROPERTIES
      PURPOSE "Gives Konqueror a plugin to speak portions or all of a website")

function (konqueror_install_plugin_in_namespaces name)
    install(TARGETS ${name} DESTINATION ${KDE_INSTALL_PLUGINDIR})
    install(FILES ${name}.rc DESTINATION ${KDE_INSTALL_DATADIR}/konqueror/partsrcfiles)

    # install the symlink in all the given nammespace, we do not want to duplicate the actual files
    set(namespace_list khtml/kpartplugins kwebkitpart/kpartplugins webenginepart/kpartplugins)
    foreach(INSTALL_NAMESPACE ${namespace_list})
        string(REPLACE "/" "_" install_namespace_file ${INSTALL_NAMESPACE})
        add_custom_command(TARGET ${name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E create_symlink ../../${name}.so ${CMAKE_CURRENT_BINARY_DIR}/${name}${install_namespace_file}.so)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${name}${install_namespace_file}.so DESTINATION ${KDE_INSTALL_PLUGINDIR}/${INSTALL_NAMESPACE})
    endforeach()

endfunction()

add_subdirectory( khtmlsettingsplugin )
add_subdirectory( kimgalleryplugin )
add_subdirectory( dirfilter )
add_subdirectory( uachanger )
add_subdirectory( babelfish )
add_subdirectory( webarchiver )
# TODO KF6: after dropping compatibility with KF5, port these plugins to AsyncSelectorInterface and
# remove the if. Since KParts::SelectorInterface is going away in KF6, supporting them in both versions
# is too difficult
if (KF_MAJOR_VERSION STRLESS "6")
add_subdirectory( kget )
add_subdirectory( akregator )
add_subdirectory( searchbar )
endif()

if (QtTextToSpeech_FOUND)
   add_subdirectory(ttsplugin)
endif()

if(UNIX)
   add_subdirectory( shellcmdplugin )
endif(UNIX)

# TODO add_subdirectory( imagerotation )
add_subdirectory( autorefresh )
if(UNIX)
   add_subdirectory( fsview )
endif()

