########### install files ###############

install(FILES
    linkProgram.desktop
    linkURL.desktop
    linkPath.desktop
    linkFloppy.desktop
    linkHD.desktop
    linkCDROM.desktop
    Directory.desktop
    TextFile.desktop
    HTMLFile.desktop
    linkZIP.desktop
    linkDVDROM.desktop
    linkCAMERA.desktop
    linkNFS.desktop
    linkCDWRITER.desktop
    linkMO.desktop
    DESTINATION ${KDE_INSTALL_TEMPLATEDIR}
)


install(FILES
    CAMERA-Device.desktop
    CDROM-Device.desktop
    CDWRITER-Device.desktop
    DVDROM-Device.desktop
    Floppy.desktop
    HD.desktop
    HTMLFile.html
    MO-Device.desktop
    NFS.desktop
    Program.desktop
    TextFile.txt
    URL.desktop
    ZIP-Device.desktop
    DESTINATION ${KDE_INSTALL_TEMPLATEDIR}/.source
)
