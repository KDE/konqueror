add_subdirectory(tests)


set(libfiletypes_SRCS
 filetypedetails.cpp
 filegroupdetails.cpp
 kservicelistwidget.cpp
 typeslistitem.cpp
 mimetypedata.cpp
 mimetypewriter.cpp
 newtypedlg.cpp
 kserviceselectdlg.cpp
)

########### next target ###############

set(kcm_filetypes_SRCS filetypesview.cpp ${libfiletypes_SRCS})

kde4_add_plugin(kcm_filetypes ${kcm_filetypes_SRCS})

target_link_libraries(kcm_filetypes  ${KDE4_KDE3SUPPORT_LIBS} )

install(TARGETS kcm_filetypes  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

set(keditfiletype_SRCS keditfiletype.cpp ${libfiletypes_SRCS})

kde4_add_executable(keditfiletype ${keditfiletype_SRCS})

target_link_libraries(keditfiletype  ${KDE4_KIO_LIBS} 
	              ${QT_QT3SUPPORT_LIBRARY}
		      ${KDE4_KDE3SUPPORT_LIBRARY})

install(TARGETS keditfiletype DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############

install( FILES filetypes.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
