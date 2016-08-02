set(searchbar_ICONS
	hi16-actions-google.png
	hi22-actions-google.png
	hi32-actions-google.png
	hi48-actions-google.png
	hi64-actions-google.png
	)
set(searchbar_ox_ICONS oxsc-actions-google.svgz)
ecm_install_icons(ICONS ${searchbar_ICONS} DESTINATION ${DATA_INSTALL_DIR}/konqueror/icons/ THEME hicolor )
ecm_install_icons(ICONS ${searchbar_ox_ICONS} DESTINATION ${DATA_INSTALL_DIR}/konqueror/icons/ THEME oxygen )
