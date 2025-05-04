/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "configdialog.h"

#include <KCModule>
#include <KLocalizedString>

#include <QTabWidget>

using namespace Konq;

Konq::ConfigDialog::ConfigDialog(QWidget* parent) : KCMultiDialog(parent)
{
    setObjectName(QStringLiteral("configureDialog"));
    setFaceType(KPageDialog::Tree);

    QMap<Module, QString> modules {
        {GeneralModule, "konqueror_kcms/khtml_general"},
        {PerformanceModule, "konqueror_kcms/kcm_performance"},
        {TabsModule, "konqueror_kcms/khtml_tabs"},
        {BookmarksModule, "konqueror_kcms/kcm_bookmarks"},
        {KonqModule, "konqueror_kcms/kcm_konq"},
        {DolphinGeneralModule, "dolphin/kcms/kcm_dolphingeneral"},
        {DolphinViewModesModule, "dolphin/kcms/kcm_dolphinviewmodes"},
        {TrashModule, "kcm_trash"},
        {FileTypesModule, "plasma/kcms/systemsettings_qwidgets/kcm_filetypes"},
        {HtmlBehaviorModule, "konqueror_kcms/khtml_behavior"},
        {HtmlAppearanceModule, "konqueror_kcms/khtml_appearance"},
        {AdBlockModule, "konqueror_kcms/khtml_filter"},
        {HtmlCacheModule, "konqueror_kcms/khtml_cache"},
        {WebShortcutsModule, "plasma/kcms/systemsettings_qwidgets/kcm_webshortcuts"},
        {ProxyModule, "plasma/kcms/systemsettings_qwidgets/kcm_proxy"},
        {HistoryModule, "konqueror_kcms/kcm_history"},
        {CookiesModule, "konqueror_kcms/khtml_cookies"},
        {JavaModule, "konqueror_kcms/khtml_java_js"},
        {UserAgentModule, "konqueror_kcms/khtml_useragent"}
    };

#ifdef Q_OS_WIN
    modules.remove(PerformanceModule);
#endif
    for (auto modIt = modules.constBegin(); modIt != modules.constEnd(); ++modIt) {
        KPluginMetaData md(modIt.value());
        //In KF6, the web shortcuts and the proxy KCMs are provided by kio-extras, which Konqueror doesn't require,
        //so avoid displaying an invalid entry if they can't be loaded for any reason
        if (!md.isValid() && (modIt.key() == WebShortcutsModule || modIt.key() == ProxyModule)) {
            continue;
        }
        KPageWidgetItem *it = addModule(md);
        //Attempt to remove the Behavior tab from the Dolphin general module, as it only applies to dolphin
        if (modIt.key() == DolphinGeneralModule) {
            fixDolphinGeneralPage(it);
        }
        m_pages[modIt.key()] = it;
    }
}

Konq::ConfigDialog::~ConfigDialog() noexcept
{
}

void Konq::ConfigDialog::fixDolphinGeneralPage(KPageWidgetItem *it)
{
    QTabWidget *tw = it->widget()->findChild<QTabWidget*>();
    if(tw && tw->count() > 0) {
        tw->removeTab(0);
    }
    QString name = i18nc("@title:tab The file manager behavior tab. Also used in the list of tabs for consistency", "File Manager Behavior");
    it->setName(name);
    it->setHeader(name);
}

QSize Konq::ConfigDialog::computeSizeHint() const
{
    QSize size{KCMultiDialog::sizeHint()};
    for (auto it = m_pages.constBegin(); it != m_pages.constEnd(); ++it) {
        QSize s = it.value()->widget()->sizeHint();
        if (size.width() < s.width()) {
            size.setWidth(s.width());
        }
        if (size.height() < s.height()) {
            size.setHeight(s.height());
        }
    }
    return size;
}

QSize Konq::ConfigDialog::sizeHint() const
{
    if (!m_sizeHint.isValid()) {
        m_sizeHint = computeSizeHint();
    }
    return m_sizeHint;
}

void Konq::ConfigDialog::setCurrentPage(Module module)
{
    KPageWidgetItem *page = m_pages.value(module);
    if (page) {
        KCMultiDialog::setCurrentPage(page);
    }
}
