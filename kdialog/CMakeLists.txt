set(kdialog_SRCS
    kdialog.cpp
    widgets.cpp
    klistboxdialog.cpp
    progressdialog.cpp)

qt4_add_dbus_adaptor( kdialog_SRCS org.kde.kdialog.ProgressDialog.xml progressdialog.h KProgressDialog )

kde4_add_executable(kdialog ${kdialog_SRCS})

# Need libkfile due to the code that adjusts the geometry of the KDirSelectDialog
target_link_libraries(kdialog ${KDE4_KFILE_LIBS})
if (Q_WS_X11)
  target_link_libraries(kdialog ${X11_X11_LIB})
endif (Q_WS_X11)

install(TARGETS kdialog ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.kdialog.ProgressDialog.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
