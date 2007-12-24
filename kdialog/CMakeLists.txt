set(kdialog_SRCS
    kdialog.cpp
    widgets.cpp
    klistboxdialog.cpp
    progressdialog.cpp)

qt4_add_dbus_adaptor( kdialog_SRCS org.kde.kdialog.ProgressDialog.xml progressdialog.h KProgressDialog )

kde4_add_executable(kdialog ${kdialog_SRCS})

# Need libkfile due to the code that adjusts the geometry of the KDirSelectDialog
target_link_libraries(kdialog ${KDE4_KFILE_LIBS})

install(TARGETS kdialog DESTINATION ${BIN_INSTALL_DIR})
install(FILES org.kde.kdialog.ProgressDialog.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
