set(searchbar_ICONS
	16-actions-google.png
	22-actions-google.png
	32-actions-google.png
	48-actions-google.png
	64-actions-google.png
	)
set(searchbar_ox_ICONS oxsc-actions-google.svgz)
ecm_install_icons(ICONS ${searchbar_ICONS} DESTINATION ${DATA_INSTALL_DIR}/konqueror/icons/ THEME hicolor )
ecm_install_icons(ICONS ${searchbar_ox_ICONS} DESTINATION ${DATA_INSTALL_DIR}/konqueror/icons/ THEME oxygen )
