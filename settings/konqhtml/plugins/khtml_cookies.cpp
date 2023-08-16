#include "cookies/kcookiesmain.h"
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(KCookiesMain, "cookies/khtml_cookies.json")

#include "khtml_cookies.moc"
