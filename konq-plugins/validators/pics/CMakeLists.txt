set(validators_hi_ICONS
  hi16-actions-validators.png
  hi22-actions-validators.png
  hi48-actions-htmlvalidator.png
  hi64-actions-validators.png
  hi16-actions-cssvalidator.png
  hi22-actions-cssvalidator.png
  hi32-actions-htmlvalidator.png
  hi48-actions-validators.png
  hi16-actions-htmlvalidator.png
  hi22-actions-htmlvalidator.png
  hi32-actions-validators.png
  hi64-actions-htmlvalidator.png
)
set(validators_ox_ICONS
  oxsc-actions-htmlvalidator.svgz
  oxsc-actions-validators.svgz
)
ecm_install_icons(ICONS ${validators_hi_ICONS} DESTINATION ${ICON_INSTALL_DIR} THEME hicolor)
ecm_install_icons(ICONS ${validators_ox_ICONS} DESTINATION ${ICON_INSTALL_DIR} THEME oxygen)
