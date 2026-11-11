# Currently, tests fail for original Plasma KCM, so wait until that's fixed to run them
# if(BUILD_TESTING)
#     add_subdirectory(tests)
# endif()

# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_konq_filetypes\")

set(libfiletypes_SRCS
 filetypedetails.cpp
 filegroupdetails.cpp
 kservicelistwidget.cpp
 typeslistitem.cpp
 typeslistitembase.cpp
 typeslistproxyitem.cpp
 typeslisttreewidget.cpp
 mimetypedata.cpp
 mimetypewriter.cpp
 multiapplydialog.cpp
 newtypedlg.cpp
 kserviceselectdlg.cpp
)

########### library ###############

add_library(libfiletypes STATIC ${libfiletypes_SRCS})
# Needed for KDE Invent CI
set_target_properties(libfiletypes PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(libfiletypes
  PUBLIC
    KF6::I18n
    KF6::KIOWidgets # KOpenWithDialog, KBuildSycocaProgressDialog
  PRIVATE
    KF6::ConfigCore
    KF6::IconThemes
    KF6::IconWidgets
    KF6::Parts
    Qt::DBus
)

########### KCM ###############

set(kcm_filetypes_SRCS filetypesview.cpp filetypesview.h)

kcoreaddons_add_plugin(kcm_konq_filetypes SOURCES ${kcm_filetypes_SRCS} INSTALL_NAMESPACE konqueror_kcms)

kcmutils_generate_desktop_file(kcm_konq_filetypes)
target_link_libraries(kcm_konq_filetypes
    libfiletypes
    KF6::KCMUtils
)

########### command line utility ###############

# set(keditfiletype_SRCS keditfiletype.cpp keditfiletype.h)
#
# add_executable(keditfiletype ${keditfiletype_SRCS})
# target_compile_definitions(keditfiletype PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")
#
# target_link_libraries(keditfiletype
#     libfiletypes
#     KF6::WindowSystem
# )
#
# install(TARGETS keditfiletype DESTINATION ${KDE_INSTALL_FULL_BINDIR})
